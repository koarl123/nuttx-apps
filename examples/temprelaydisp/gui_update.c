#include "myview_target.h"

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "lvgl/lvgl.h"
#include "lvgl_myview.h"

struct gui_objs
{
    lv_obj_t *meter_obj;
    lv_meter_scale_t *scale;
    lv_meter_indicator_t *indic_temp_probe;
    lv_meter_indicator_t *indic_temp_env;
    lv_obj_t *label_list;              // label(list) gui element data
    lv_obj_t *lv_switch;
    const struct autoboiler_data *autob_dat_p; // pointer to input data
};

struct gui_objs lv_gui_objs;

sem_t sem_lvgl;

static void set_lbl_text_and_vals(lv_obj_t *lbl, int temp_env, int temp_probe)
{
    lv_label_set_text_fmt(lbl, "Env:   %3i\r\nProbe: %3i\r\n",
                          temp_env,
                          temp_probe);
}

static void label_list_changed_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *lbl = lv_event_get_target(e);
    struct autoboiler_data *autob_data = lv_event_get_param(e);
    if (code == LV_EVENT_REFRESH)
    {
        set_lbl_text_and_vals(lbl, autob_data->temp_env,
                              autob_data->temp_probe);
    }
}

static void meter_changed_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    struct gui_objs *gui_p = lv_event_get_param(e);
    if (code == LV_EVENT_REFRESH)
    {
        lv_meter_set_indicator_end_value(obj, gui_p->indic_temp_probe, gui_p->autob_dat_p->temp_probe);
        lv_meter_set_indicator_end_value(obj, gui_p->indic_temp_env, gui_p->autob_dat_p->temp_env);
    }
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
    lv_obj_t *sw = priv->lv_switch;
    for (;;)
    {
        nxsem_wait(&sem_lvgl);
        //  do not directly update values, send event
        // label only needs pointer to input data
        lv_event_send(lbl, LV_EVENT_REFRESH, (void*) priv->autob_dat_p);
        // meter needs reference to indicator, send gui_objs pointer
        lv_event_send(meter, LV_EVENT_REFRESH, priv);
        // send refresh event to update shown data
        lv_event_send(sw, LV_EVENT_REFRESH, (void*)priv->autob_dat_p);
        nxsem_post(&sem_lvgl);
        //usleep(200000);
    }
    ginfo("INFO: __FUNCTION__ exiting\n");
    return NULL;
}
static void myview_init_label(void)
{
    lv_obj_t *lbl = lv_label_create(lv_scr_act());                                       /*Add a button the current screen*/
    lv_obj_set_pos(lbl, 30, 240);                                                        /*Set its position*/
    lv_obj_set_size(lbl, 120, 120);                                                      /*Set its size*/

    // default initialization of gyro values
    set_lbl_text_and_vals(lbl, 15, 40);

    lv_gui_objs.label_list = lbl;

    lv_obj_add_event_cb(lbl, label_list_changed_event_cb, LV_EVENT_ALL, (void*) lv_gui_objs.autob_dat_p); /*Assign a callback to the gyro values*/
}


static void switch_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    struct autoboiler_data *autob_data = lv_event_get_user_data(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
        LV_LOG_USER("State: %s\n", state ? "On" : "Off");
        // write back new state
        autob_data->relay_on = state;
    }
    if(code == LV_EVENT_REFRESH)
    {
        if(autob_data->relay_on)
        {
            lv_obj_add_state(obj, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_clear_state(obj, LV_STATE_CHECKED);
        }
    }
}

static void myview_init_switch( void )
{
    lv_obj_t * sw = lv_switch_create(lv_scr_act());

    lv_obj_set_pos(sw, 150, 280); /*Set its position*/

    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(sw, switch_event_handler, LV_EVENT_ALL, (void*) lv_gui_objs.autob_dat_p);

    lv_gui_objs.lv_switch = sw;

    // label the switch
    lv_obj_t *lbl = lv_label_create(lv_scr_act());                                       /*Add a button the current screen*/
    lv_obj_set_pos(lbl, 30, 290);                                                        /*Set its position*/
    lv_obj_set_size(lbl, 120, 120);                                                      /*Set its size*/
    lv_label_set_text_fmt(lbl, "Relay Switch:");
}

static void myview_init_meter(void)
{
    const int temp_min = 10;
    const int temp_lim_cold = 30;
    const int temp_lim_hot = 90;
    const int temp_max = 110;
    lv_obj_t *meter = lv_meter_create(lv_scr_act());
    lv_obj_set_pos(meter, 10, 55); /*Set its position*/
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

    lv_gui_objs.meter_obj = meter;
    lv_gui_objs.scale = scale;
    lv_gui_objs.indic_temp_env = indic2;
    lv_gui_objs.indic_temp_probe = indic1;

    lv_obj_add_event_cb(meter, meter_changed_event_cb, LV_EVENT_ALL, &lv_gui_objs); /*Assign a callback to the gyro values*/

}

#define DROPDOWN_OPTIONS_STRING "70\n80\n90\n100"
static const int DropDownOptionsTable[] =
{
    70, 80, 90, 100
};

static int GetTempFromIndex(int index)
{
    if(index > sizeof(DropDownOptionsTable))
    {
        gerr("ERROR: index out of bounds\n");
    }
    else
    {
        return DropDownOptionsTable[index];
    }
    return -1;
}

static void dropdown_changed_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dd = lv_event_get_target(e);
    struct autoboiler_data *autob_data = lv_event_get_user_data(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t index = lv_dropdown_get_selected(dd);
        autob_data->target_temp = GetTempFromIndex(index);
        lv_dropdown_set_selected(dd, index);
        lv_dropdown_set_text(dd, NULL);
    }
}

static void myview_init_targettemp_dropdown(void)
{
    lv_obj_t * dd = lv_dropdown_create(lv_scr_act());
    lv_obj_set_pos(dd, 120, 10); /*Set its position*/
    lv_obj_set_size(dd, 100, 40);
    lv_dropdown_set_text(dd, "select\r\n");
    lv_obj_add_flag(dd, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dd, LV_OBJ_FLAG_HIDDEN);
    lv_dropdown_set_options(dd, DROPDOWN_OPTIONS_STRING);
    lv_obj_add_event_cb(dd, dropdown_changed_event_cb, LV_EVENT_ALL,(void*) lv_gui_objs.autob_dat_p);

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Boil to: \n");
    lv_obj_set_pos(label, 30, 20);
    lv_obj_set_size(label, 180, 40);
}

void initialize_gui_update(struct autoboiler_data * autob_dat)
{
    pthread_t thread;
    nxsem_init(&sem_lvgl, 0, 1);
    nxsem_wait(&sem_lvgl);
    lv_gui_objs.autob_dat_p = autob_dat;
    // initialize before starting thread to avoid calling LVGL twice at the same time
    myview_init_label();
    /* create the switch for the relay*/
    myview_init_switch();
    /* initialize the meter */
    myview_init_meter();

    myview_init_targettemp_dropdown();
    nxsem_post(&sem_lvgl);

    /* start the GUI thread */
    int ret = pthread_create(&thread, NULL, gui_workerthread, (void *)&lv_gui_objs);
    if (ret != OK)
    {
        gerr("ERROR: failed to initialize gui pthread\n");
    }
}