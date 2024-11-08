#include "myview_target.h"

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "lvgl/lvgl.h"
#include "lvgl_myview.h"

struct gui_objs
{
    lv_obj_t *scale_obj;
    lv_obj_t *label_list;              // label(list) gui element data
    lv_obj_t *lv_switch;
    lv_obj_t *arc_obj;
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
    lv_obj_t *scale = priv->scale_obj;
    lv_obj_t *sw = priv->lv_switch;
    lv_obj_t *arc = priv->arc_obj;
    for (;;)
    {
        nxsem_wait(&sem_lvgl);
        //  do not directly update values, send event
        // label only needs pointer to input data
        lv_obj_send_event(lbl, LV_EVENT_REFRESH, (void*) priv->autob_dat_p);
        // scale needs reference to indicator, send gui_objs pointer
        lv_obj_send_event(scale, LV_EVENT_REFRESH, (void*) priv->autob_dat_p);
        // send refresh event to update shown data
        lv_obj_send_event(sw, LV_EVENT_REFRESH, (void*)priv->autob_dat_p);

        lv_obj_send_event(arc, LV_EVENT_REFRESH, (void*)priv->autob_dat_p);
        nxsem_post(&sem_lvgl);
        usleep(100);
    }
// TODO
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

static void scale_event_handler(lv_event_t * e)
{
    lv_scale_section_t *arc = lv_event_get_user_data(e);
    const struct autoboiler_data *autb_data = lv_event_get_param(e);
    lv_scale_section_set_range(arc, 0, autb_data->temp_probe);
}

static void value_changed_event_cb(lv_event_t *e)
{
    lv_obj_t * arc = lv_event_get_target(e);
    const struct autoboiler_data *autb_data = lv_event_get_param(e);
    int32_t temp = autb_data->temp_probe;
    if(temp > 0)
    {
        uint32_t angle = temp * 180 / 80 ;
        lv_arc_set_bg_angles(arc,0, angle);
    }
    
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

static void myview_init_scale(void)
{
    lv_obj_t * scale = lv_scale_create(lv_screen_active());
    lv_obj_set_size(scale, 150, 150);
    lv_scale_set_label_show(scale, true);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_OUTER);
    lv_obj_center(scale);

    lv_scale_set_total_tick_count(scale, 31);
    lv_scale_set_major_tick_every(scale, 5);

    lv_obj_set_style_length(scale, 7, LV_PART_ITEMS);
    lv_obj_set_style_length(scale, 10, LV_PART_INDICATOR);
    lv_scale_set_range(scale, 0, 120);

    static const char * custom_labels[] = {"0 °C", "20 °C", "40 °C", "60 °C", "80 °C", "100 °C", "120 °C", NULL};
    lv_scale_set_text_src(scale, custom_labels);

    static lv_style_t indicator_style;
    lv_style_init(&indicator_style);

    /* Label style properties */
    lv_style_set_text_font(&indicator_style, LV_FONT_DEFAULT);
    lv_style_set_text_color(&indicator_style, lv_palette_darken(LV_PALETTE_BLUE, 3));

    /* Major tick properties */
    lv_style_set_line_color(&indicator_style, lv_palette_darken(LV_PALETTE_BLUE, 3));
    lv_style_set_width(&indicator_style, 10U);      /*Tick length*/
    lv_style_set_line_width(&indicator_style, 2U);  /*Tick width*/
    lv_obj_add_style(scale, &indicator_style, LV_PART_INDICATOR);

    static lv_style_t minor_ticks_style;
    lv_style_init(&minor_ticks_style);
    lv_style_set_line_color(&minor_ticks_style, lv_palette_lighten(LV_PALETTE_BLUE, 2));
    lv_style_set_width(&minor_ticks_style, 5U);         /*Tick length*/
    lv_style_set_line_width(&minor_ticks_style, 2U);    /*Tick width*/
    lv_obj_add_style(scale, &minor_ticks_style, LV_PART_ITEMS);

    static lv_style_t main_line_style;
    lv_style_init(&main_line_style);
    /* Main line properties */
    lv_style_set_arc_color(&main_line_style, lv_palette_darken(LV_PALETTE_BLUE, 3));
    lv_style_set_arc_width(&main_line_style, 2U); /*Tick width*/
    lv_obj_add_style(scale, &main_line_style, LV_PART_MAIN);

    /* Add a section */
    static lv_style_t style_red_minor_tick_style;
    static lv_style_t style_red_label_style;
    static lv_style_t style_red_main_line_style;

    lv_style_init(&style_red_label_style);
    lv_style_init(&style_red_minor_tick_style);
    lv_style_init(&style_red_main_line_style);

    /* Label style properties */
    lv_style_set_text_font(&style_red_label_style, LV_FONT_DEFAULT);
    lv_style_set_text_color(&style_red_label_style, lv_palette_darken(LV_PALETTE_RED, 3));

    lv_style_set_line_color(&style_red_label_style, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_style_set_line_width(&style_red_label_style, 4U); /*Tick width*/

    lv_style_set_line_color(&style_red_minor_tick_style, lv_palette_lighten(LV_PALETTE_RED, 2));
    lv_style_set_line_width(&style_red_minor_tick_style, 3U); /*Tick width*/

    /* Main line properties */
    lv_style_set_arc_color(&style_red_main_line_style, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_style_set_arc_width(&style_red_main_line_style, 3U); /*Tick width*/

    /* Configure section styles */
    lv_scale_section_t * redSection = lv_scale_add_section(scale);
    lv_scale_section_set_range(redSection, 90, 120);
    lv_scale_section_set_style(redSection, LV_PART_INDICATOR, &style_red_label_style);
    lv_scale_section_set_style(redSection, LV_PART_ITEMS, &style_red_minor_tick_style);
    lv_scale_section_set_style(redSection, LV_PART_MAIN, &style_red_main_line_style);
    
    //static lv_style_t section_arc_style;
    //lv_style_init(&section_arc_style);
    ///* arc properties */
    //lv_style_set_arc_color(&section_arc_style, lv_palette_darken(LV_PALETTE_BLUE, 3));
    //lv_style_set_arc_width(&section_arc_style, 8U); /*Tick width*/

    //lv_scale_section_t * arcSection = lv_scale_add_section(scale);
    //lv_scale_section_set_style(arcSection, LV_PART_INDICATOR, &section_arc_style);
    //lv_scale_section_set_range(arcSection, 0, 10);
    //lv_obj_add_event_cb(scale, scale_event_handler, LV_EVENT_ALL, (void*) arcSection);

    lv_gui_objs.scale_obj = scale;

    lv_obj_t * arc = lv_arc_create(scale);
    lv_obj_set_size(arc, 150, 150);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_center(arc);
    lv_obj_add_event_cb(arc, value_changed_event_cb, LV_EVENT_REFRESH, (void*) lv_gui_objs.autob_dat_p);

    lv_gui_objs.arc_obj = arc;
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
    /* initialize the scale */
    myview_init_scale();

    myview_init_targettemp_dropdown();
    nxsem_post(&sem_lvgl);
    
    /* start the GUI thread */
    int ret = pthread_create(&thread, NULL, gui_workerthread, (void *)&lv_gui_objs);
    if (ret != OK)
    {
        gerr("ERROR: failed to initialize gui pthread\n");
    }
}