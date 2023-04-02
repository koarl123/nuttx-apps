#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <nuttx/sensors/sensor.h>
#include <sys/ioctl.h>
//#include <pthread.h>
#include <fcntl.h>
#include <debug.h>
#include "lvgl_myview.h"
#include "lvgl/lvgl.h"

#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

struct input_data
{
struct sensor_gyro gyro_data;
struct adc_msg_s adc_data;
};

static pthread_t gui_thread;
static sem_t sem_input_dat;

static float calcResistorValueFromAdc(float adc)
{
    int R1 = 4900; // Ohm
    float Rtemp = ((4096 - adc) *R1) / adc;
    return Rtemp;
}

static void gyro_set_lbl_text_and_vals( lv_obj_t * lbl, float x, float y, float z, float temperature, int adc )
{
    lv_label_set_text_fmt(lbl, "x %+2.2f\r\ny %+2.2f\r\nz %+2.2f\r\nt %+2.2f\r\na %d\r\nR %4.2f\r\n",
                                x, y, z, temperature, adc, calcResistorValueFromAdc(adc));
}

static void label_list_changed_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * lbl = lv_event_get_target(e);
    nxsem_wait(&sem_input_dat);
    struct input_data * input_data = lv_event_get_param(e);
    if(code == LV_EVENT_VALUE_CHANGED)
    {
        gyro_set_lbl_text_and_vals(lbl, input_data->gyro_data.x,
                                        input_data->gyro_data.y,
                                        input_data->gyro_data.z,
                                        input_data->gyro_data.temperature,
                                        input_data->adc_data.am_data);
    }
    nxsem_post(&sem_input_dat);
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Clicks %d", cnt);
    }
}

/**
 * Create a button with a label and react on click event.
 */
static void myview_centered_button(void)
{
    lv_obj_t * btn = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/
    lv_obj_set_pos(btn, 60, 40);                            /*Set its position*/
    lv_obj_set_size(btn, 80, 60);                          /*Set its size*/
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Clicks");                     /*Set the labels text*/
    lv_obj_center(label);
}

static void * gui_workerthread( void * arg)
{
    int fd_gyro, fd_adc;
    int ret;
    lv_obj_t * lbl = (lv_obj_t *) arg;

    fd_gyro = open("/dev/uorb/sensor_gyro_uncal0", O_RDONLY);
    fd_adc = open("/dev/adc0", O_RDONLY);
    if(fd_gyro < 0)
    {
      gerr("ERROR: Failed to open sensor: %d\n", errno);
      return NULL;
    }
    struct input_data input_data;
    for(;;)
    {
        bool gyro_ok, adc_ok;
        usleep(1000);
        //blocking call to read on gyro sensor file descriptor.
        nxsem_wait(&sem_input_dat);
        ssize_t size = read(fd_gyro, &input_data.gyro_data, sizeof(struct sensor_gyro));
        nxsem_post(&sem_input_dat);
        if(size == sizeof(struct sensor_gyro))
        {
            gyro_ok = true;
        }
        else
        {
            gyro_ok = false;
            gwarn("WARNING: size of sensor data mismatch\n");
            break;
        }
        ret = ioctl(fd_adc, ANIOC_TRIGGER, 0);
        if(ret < 0)
        {
            int errcode = errno;
            adc_ok = false;
            gwarn("adc ANIOC_TRIGGER ioctl failed: %d\n", errcode);
        }
        else
        {
            // now get ADC data
            nxsem_wait(&sem_input_dat);
            int nbytes = read(fd_adc, &input_data.adc_data, sizeof(struct adc_msg_s));
            nxsem_post(&sem_input_dat);
            if(nbytes == sizeof(struct adc_msg_s))
            {
                adc_ok = true;
            }
            else
            {
                adc_ok = false;
            }
        }
        if( gyro_ok || adc_ok )
        {
            // do not directly update values
            lv_event_send(lbl, LV_EVENT_VALUE_CHANGED, &input_data);
        }


    }
    ginfo("INFO: __FUNCTION__ exiting\n");
    return NULL;
}

static void ly_myview_initialize( void )
{
    pthread_attr_t tattr;
    struct sched_param sparam;
    int ret;
    lv_obj_t * lbl = lv_label_create(lv_scr_act());     /*Add a button the current screen*/
    lv_obj_set_pos(lbl, 30, 160);                    /*Set its position*/
    lv_obj_set_size(lbl, 180, 80);                      /*Set its size*/
    lv_obj_add_event_cb(lbl, label_list_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL); /*Assign a callback to the gyro values*/
    nxsem_init(&sem_input_dat, 0, 1);
    // default initialization of gyro values
    gyro_set_lbl_text_and_vals(lbl, 0.1, 0.2, 0.3, 10.0, 1024);

    /* Start our thread for sending data to the device */

    ret = pthread_attr_init(&tattr);
    if( ret != OK)
    {
        gerr("ERROR: failed to initialize gui thread attr\n");
    }
    sparam.sched_priority = 0x64; // low priority
    ret = pthread_attr_setschedparam(&tattr, &sparam);
    if( ret != OK)
    {
        gerr("ERROR: failed to initialize gui thread attr\n");
    }

    ret = pthread_attr_setstacksize(&tattr, 0x800);
    if( ret != OK)
    {
        gerr("ERROR: failed to initialize gui thread attr\n");
    }

    ret = pthread_create(&gui_thread, &tattr, gui_workerthread, (void*) lbl);
    if( ret != OK)
    {
        gerr("ERROR: failed to initialize gui pthread\n");
    }
}

/* initialize my mega view */
void lv_myview(void)
{
    /* centered button, as in example */
    myview_centered_button();
    /* next level: show gyro MEMS float values that is updated*/
    ly_myview_initialize();
}