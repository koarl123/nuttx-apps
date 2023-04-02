
#undef SIMULATOR
// #define SIMULATOR 1
#ifdef SIMULATOR
#include "myview_simulator.h"
#include "../../lv_demo.h"

#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#else
#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <nuttx/sensors/sensor.h>
#include <sys/ioctl.h>

#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

#include <debug.h>
#endif

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "lvgl_myview.h"
#include "lvgl/lvgl.h"

// data from sensors
struct input_data
{
    int temp_env;
    int temp_probe;
};

struct gui_objs
{
    lv_obj_t *meter_obj;
    lv_meter_scale_t *scale;
    lv_meter_indicator_t *indic_temp_probe;
    lv_meter_indicator_t *indic_temp_env;
    lv_obj_t *label_list;              // label(list) gui element data
    const struct input_data *in_dat_p; // pointer to input data
};

struct myview_priv
{
    struct gui_objs lv_gui_objs;     // meter and label gui element data
    pthread_t gui_thread;            // thread for updating GUI data
    pthread_t data_collector_thread; // thread for data collection: get gyro data, get and filter ADC data
    sem_t sem_input_dat;             // semaphore protecting access to struct input_data
    struct input_data input_data;    // data to visualize
};

static struct myview_priv mv_priv;
sem_t sem_lvgl;

#define ADC_FILTER_BUF_SIZE 20

#define PT1000TAB_TEMP_MIN -50
#define PT1000TAB_TEMP_MAX 259
#define PT1000TAB_ENTRIES 310
// table from -50 degree up to 259 degree celsius
static float Pt1000TemperatureTable[PT1000TAB_ENTRIES] =
    {
        803.1,
        807.00,
        811.00,
        815.00,
        818.90,
        822.90,
        826.90,
        830.80,
        834.80,
        838.80,
        842.90,
        846.70,
        850.60,
        854.60,
        858.50,
        862.50,
        866.40,
        870.40,
        874.30,
        878.30,
        822.20,
        886.20,
        890.10,
        894.00,
        898.00,
        901.90,
        905.90,
        909.80,
        913.70,
        917.70,
        921.60,
        925.50,
        929.50,
        933.40,
        937.30,
        941.20,
        945.20,
        949.10,
        953.00,
        956.90,
        960.90,
        964.80,
        968.70,
        972.60,
        976.50,
        980.40,
        984.40,
        988.30,
        992.20,
        996.10,
        1000,
        1003.9,
        1007.8,
        1011.7,
        1015.6,
        1019.5,
        1023.4,
        1027.3,
        1031.2,
        1035.1,
        1039,
        1042.9,
        1046.8,
        1050.7,
        1054.6,
        1058.5,
        1062.4,
        1066.3,
        1070.2,
        1074,
        1077.9,
        1081.8,
        1085.7,
        1089.6,
        1093.5,
        1097.3,
        1101.2,
        1105.1,
        1109,
        1112.8,
        1116.7,
        1120.6,
        1124.5,
        1128.3,
        1132.2,
        1136.1,
        1139.9,
        1143.8,
        1147.7,
        1151.5,
        1155.4,
        1159.3,
        1163.1,
        1167,
        1170.8,
        1174.7,
        1178.5,
        1182.4,
        1186.2,
        1190.1,
        1194,
        1197.8,
        1201.6,
        1205.5,
        1209.3,
        1213.2,
        1217,
        1220.9,
        1224.7,
        1228.6,
        1232.4,
        1236.2,
        1240.1,
        1243.9,
        1247.7,
        1251.6,
        1255.4,
        1259.2,
        1263.1,
        1266.9,
        1270.7,
        1274.5,
        1278.4,
        1282.2,
        1286,
        1289.8,
        1293.7,
        1297.5,
        1301.3,
        1305.1,
        1308.9,
        1312.7,
        1316.6,
        1320.4,
        1324.2,
        1328,
        1331.8,
        1335.6,
        1339.4,
        1343.2,
        1347,
        1350.8,
        1354.6,
        1358.4,
        1362.2,
        1366,
        1369.8,
        1373.6,
        1377.4,
        1381.2,
        1385,
        1388.8,
        1392.6,
        1396.4,
        1400.2,
        1403.9,
        1407.7,
        1411.5,
        1415.3,
        1419.1,
        1422.9,
        1426.6,
        1430.4,
        1434.2,
        1438,
        1441.7,
        1445.5,
        1449.3,
        1453.1,
        1456.8,
        1460.6,
        1464.4,
        1468.1,
        1471.9,
        1475.7,
        1479.4,
        1483.2,
        1487,
        1490.7,
        1494.5,
        1498.2,
        1502,
        1505.7,
        1509.5,
        1513.3,
        1517,
        1520.8,
        1524.5,
        1528.3,
        1532,
        1535.8,
        1539.5,
        1543.2,
        1547,
        1550.7,
        1554.5,
        1558.2,
        1561.9,
        1565.7,
        1569.4,
        1573.1,
        1576.9,
        1580.6,
        1584.3,
        1588.1,
        1591.8,
        1595.5,
        1599.3,
        1603,
        1606.7,
        1610.4,
        1614.2,
        1617.9,
        1621.6,
        1625.3,
        1629,
        1632.7,
        1636.5,
        1640.2,
        1643.9,
        1647.6,
        1651.3,
        1655,
        1658.7,
        1662.4,
        1666.1,
        1669.8,
        1673.5,
        1677.2,
        1680.9,
        1684.6,
        1688.3,
        1692,
        1695.7,
        1699.4,
        1703.1,
        1706.8,
        1710.5,
        1714.2,
        1711.9,
        1721.6,
        1725.3,
        1729,
        1732.6,
        1736.3,
        1740,
        1743.7,
        1747.4,
        1751,
        1754.7,
        1758.4,
        1762.1,
        1765.7,
        1769.4,
        1773.1,
        1776.8,
        1780.4,
        1784.1,
        1787.8,
        1791.4,
        1795.1,
        1798.8,
        1802.4,
        1806.1,
        1809.7,
        1813.4,
        1817.1,
        1820.7,
        1824.4,
        1828,
        1831.7,
        1835.3,
        1839,
        1842.6,
        1846.3,
        1849.9,
        1853.6,
        1857.2,
        1860.9,
        1864.5,
        1868.2,
        1871.8,
        1875.4,
        1879.1,
        1882.7,
        1886.3,
        1890,
        1893.6,
        1897.2,
        1900.9,
        1904.5,
        1908.1,
        1911.8,
        1915.4,
        1919,
        1922.6,
        1926.3,
        1929.9,
        1933.5,
        1937.1,
        1940.7,
        1944.4,
        1948,
        1951.6,
        1955.2,
        1958.8,
        1962.4,
        1966,
        1969.6,
        1973.3,
};

static void set_lbl_text_and_vals(lv_obj_t *lbl, int temp_env, int temp_probe)
{
    lv_label_set_text_fmt(lbl, "Env:   %i\r\nProbe: %i\r\n",
                          temp_env,
                          temp_probe);
}

static void label_list_changed_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *lbl = lv_event_get_target(e);
    // nxsem_wait(&priv.sem_input_dat);
    struct input_data *input_data = lv_event_get_param(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        set_lbl_text_and_vals(lbl, input_data->temp_env,
                              input_data->temp_probe);
    }
    // nxsem_post(&priv.sem_input_dat);
}

static void btn_event_cb(lv_event_t *e)
{
    // function is called by LVGL, semaphore already taken
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        static uint8_t cnt = 0;
        cnt++;
        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Clicks %d", cnt);
    }
}

static void meter_changed_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    struct gui_objs *gui_p = lv_event_get_param(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_meter_set_indicator_end_value(obj, gui_p->indic_temp_probe, gui_p->in_dat_p->temp_probe);
        lv_meter_set_indicator_end_value(obj, gui_p->indic_temp_env, gui_p->in_dat_p->temp_env);
    }
}

/**
 * Create a button with a label and react on click event.
 */
static void myview_centered_button(void)
{
    lv_obj_t *btn = lv_btn_create(lv_scr_act()); /*Add a button the current screen*/
    lv_obj_set_pos(btn, 310, 10);                 /*Set its position*/
    lv_obj_set_size(btn, 20, 20);                /*Set its size*/
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL); /*Assign a callback to the button*/

    lv_obj_t *label = lv_label_create(btn); /*Add a label to the button*/
    lv_label_set_text(label, "Clicks");     /*Set the labels text*/
    lv_obj_center(label);
}

static float calcResistorValueFromAdc(float adc)
{
    int R1 = 729; // Ohm
    float Rtemp = ((4096 - adc) * R1) / adc;
    return Rtemp;
}

static int GetProbeTempFromAdcVal(int adc_data)
{
    float Rt = calcResistorValueFromAdc(adc_data);
    for (uint32_t i = 0; i < PT1000TAB_ENTRIES - 1; i++)
    {
        if (Rt < Pt1000TemperatureTable[i + 1] && Rt > Pt1000TemperatureTable[i])
        {
            return (i - 50);
        }
    }
    // default return
    gwarn("value not found\n");
    return 0;
}

int GetFilteredTempFromAdc(int adc_data_raw)
{
    // filter ADC data
    static int adc_buffer[ADC_FILTER_BUF_SIZE];
    static int adc_buf_put = 0;
    static bool adc_filled = false;
    // add data to buffer
    adc_buffer[adc_buf_put++] = adc_data_raw;
    if (adc_buf_put >= ADC_FILTER_BUF_SIZE)
    {
        adc_buf_put = 0;
        adc_filled = true;
    }

    // calculate mean
    int sum = 0;
    for (int i = 0; i < ADC_FILTER_BUF_SIZE; i++)
    {
        sum += adc_buffer[i];
    }
    int adc_filtered = sum / ADC_FILTER_BUF_SIZE;
    if(adc_filled)
    {
        return GetProbeTempFromAdcVal(adc_filtered);
    }
    else
    {
        return 0; // degrees
    }
}

// THREAD: collect data from sensor, filter them and give them to internal priv data
static void *data_collector_thread(void *arg)
{
    struct input_data *priv = (struct input_data *)arg;
    if(priv==NULL)
    {
        gerr("error in thread __FUNCTION__\n");
        return NULL;
    }
#ifdef SIMULATOR
    const char *adcfilepath = "/tmp/dev/adc0";
    const char *gyrofilepath = "/tmp/dev/uorb/sensor_gyro_uncal0";
    mkfifo(gyrofilepath, 0666);
    mkfifo(adcfilepath, 0666);
#else
    const char *adcfilepath = "/dev/adc0";
    const char *gyrofilepath = "/dev/uorb/sensor_gyro_uncal0";
#endif

    int fd_gyro = open(gyrofilepath, O_RDONLY);
    int fd_adc = open(adcfilepath, O_RDONLY);

    if (fd_gyro < 0 || fd_adc < 0)
    {
        gerr("ERROR: Failed to open sensor: %d\n", errno);
        return NULL;
    }
    for (;;)
    {
        struct sensor_gyro gyro_data;
        // blocking call to read on gyro sensor file descriptor.
        ssize_t size = read(fd_gyro, &gyro_data, sizeof(struct sensor_gyro));
        if (size != sizeof(struct sensor_gyro))
        {
            gwarn("WARNING: size of gyro data mismatch\n");
            break;
        }
        // nxsem_wait(&priv.sem_input_dat);
        priv->temp_env = gyro_data.temperature;
        // give gyro data to priv
        // nxsem_post(&priv.sem_input_dat);

        // now get ADC data
        int ret = ioctl(fd_adc, ANIOC_TRIGGER, 0);
        if (ret < 0)
        {
            int errcode = errno;
            gwarn("adc ANIOC_TRIGGER ioctl failed: %d\n", errcode);
        }
        else
        {
            // now get ADC data
            struct adc_msg_s adc_raw;
            int nbytes = read(fd_adc, &adc_raw, sizeof(struct adc_msg_s));
            if (nbytes != sizeof(struct adc_msg_s))
            {
                gwarn("WARNING: size of adc data mismatch\n");
                break;
            }
            int temp_probe = GetFilteredTempFromAdc(adc_raw.am_data);
            // nxsem_wait(&priv.sem_input_dat);
            //  provide ADC data to priv structure
            priv->temp_probe = temp_probe;
            // nxsem_post(&priv.sem_input_dat);
        }
    }
    return NULL;
}

// THREAD: update LV GUI objects with data from data collector
// make sure LVGL is only accessed when we have lvgl semaphore
static void *gui_workerthread(void *arg)
{
    struct gui_objs *priv = (struct gui_objs *)arg;
    if(priv==NULL)
    {
        gerr("error in thread __FUNCTION__\n");
        return NULL;
    }
    lv_obj_t *lbl = priv->label_list;
    lv_obj_t *meter = priv->meter_obj;
    for (;;)
    {
        nxsem_wait(&sem_lvgl);
        // nxsem_wait(&priv.sem_input_dat);
        //  do not directly update values, send event
        // label only needs pointer to input data
        lv_event_send(lbl, LV_EVENT_VALUE_CHANGED, (void*) priv->in_dat_p);
        // meter needs reference to indicator, send gui_objs pointer
        lv_event_send(meter, LV_EVENT_VALUE_CHANGED, priv);
        nxsem_post(&sem_lvgl);
        // nxsem_post(&priv.sem_input_dat);
        usleep(200000);
    }
    ginfo("INFO: __FUNCTION__ exiting\n");
    return NULL;
}

static void myview_init_label(void)
{
    lv_obj_t *lbl = lv_label_create(lv_scr_act());                                       /*Add a button the current screen*/
    lv_obj_set_pos(lbl, 60, 190);                                                        /*Set its position*/
    lv_obj_set_size(lbl, 120, 120);                                                      /*Set its size*/

    // default initialization of gyro values
    set_lbl_text_and_vals(lbl, 15, 40);

    mv_priv.lv_gui_objs.label_list = lbl;

    lv_obj_add_event_cb(lbl, label_list_changed_event_cb, LV_EVENT_VALUE_CHANGED, &mv_priv.input_data); /*Assign a callback to the gyro values*/

}

static void myview_init_meter(void)
{
    const int temp_min = 10;
    const int temp_lim_cold = 30;
    const int temp_lim_hot = 90;
    const int temp_max = 110;
    lv_obj_t *meter = lv_meter_create(lv_scr_act());
    // lv_obj_center(meter);
    lv_obj_set_pos(meter, 10, 10); /*Set its position*/
    lv_obj_set_size(meter, 180, 180);

    /*Remove the circle from the middle*/
    lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);

    /*Add a scale first*/
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);

    /*Add a scale first*/
    lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 4, 4, 15, lv_color_black(), 10);
    lv_meter_set_scale_range(meter, scale, temp_min, temp_max, 270, 135);

    /*Add two arc indicators*/
    lv_meter_indicator_t *indic1 = lv_meter_add_arc(meter, scale, 5, lv_palette_main(LV_PALETTE_LIGHT_BLUE), -5);
    lv_meter_set_indicator_start_value(meter, indic1, 0);
    lv_meter_set_indicator_end_value(meter, indic1, 120);

    lv_meter_indicator_t *indic2 = lv_meter_add_arc(meter, scale, 5, lv_palette_main(LV_PALETTE_LIGHT_GREEN), -10);
    lv_meter_set_indicator_start_value(meter, indic2, 0);
    lv_meter_set_indicator_end_value(meter, indic2, 40);

    lv_meter_indicator_t *indic3 = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_meter_set_indicator_start_value(meter, indic3, temp_lim_cold);
    lv_meter_set_indicator_end_value(meter, indic3, temp_lim_hot);

    /*Add a blue arc to the start*/
    lv_meter_indicator_t *indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, indic, temp_min);
    lv_meter_set_indicator_end_value(meter, indic, temp_lim_cold);

    /*Make the tick lines blue at the start of the scale*/
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE),
                                     false, 0);
    lv_meter_set_indicator_start_value(meter, indic, temp_min);
    lv_meter_set_indicator_end_value(meter, indic, temp_lim_cold);

    /*Add a red arc to the end*/
    indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(meter, indic, temp_lim_hot);
    lv_meter_set_indicator_end_value(meter, indic, temp_max);

    /*Make the tick lines red at the end of the scale*/
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false,
                                     0);
    lv_meter_set_indicator_start_value(meter, indic, temp_lim_hot);
    lv_meter_set_indicator_end_value(meter, indic, temp_max);

    mv_priv.lv_gui_objs.meter_obj = meter;
    mv_priv.lv_gui_objs.scale = scale;
    mv_priv.lv_gui_objs.indic_temp_env = indic2;
    mv_priv.lv_gui_objs.indic_temp_probe = indic1;

    lv_obj_add_event_cb(meter, meter_changed_event_cb, LV_EVENT_VALUE_CHANGED, &mv_priv.lv_gui_objs); /*Assign a callback to the gyro values*/

}

/* initialize my mega view */
void lv_myview(void)
{
    int ret;
    nxsem_init(&sem_lvgl, 0, 1);
    nxsem_init(&mv_priv.sem_input_dat, 0, 1);

    mv_priv.lv_gui_objs.in_dat_p = &mv_priv.input_data;
    nxsem_wait(&sem_lvgl);
    // initialize before starting thread to avoid calling LVGL twice at the same time
    myview_init_label();

    /* centered button, as in example */
    myview_centered_button();

    /* initialize the meter */
    myview_init_meter();


    nxsem_post(&sem_lvgl);

#ifdef SIMULATOR
    myview_sim_initialize_sensors();
#endif
    /* start the GUI thread */
    ret = pthread_create(&mv_priv.gui_thread, NULL, gui_workerthread, (void *)&mv_priv.lv_gui_objs);
    if (ret != OK)
    {
        gerr("ERROR: failed to initialize gui pthread\n");
    }
    /* start thread for data collection from sensors */
    ret = pthread_create(&mv_priv.data_collector_thread, NULL, data_collector_thread, (void *)&mv_priv.input_data);
    if (ret != OK)
    {
        gerr("ERROR: failed to initialize data collector pthread\n");
    }
}