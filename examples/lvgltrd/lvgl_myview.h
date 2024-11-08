/**
 * @file lvgl_myview.h
 *
 */

#ifndef LVGL_MYVIEW_H__
#define LVGL_MYVIEW_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
// data from sensors
struct autoboiler_data
{
    int temp_env;
    int temp_probe;
    bool relay_on;
    int target_temp;
};
/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_myview(void);
void initialize_data_collection( struct autoboiler_data * autob_dat);
void initialize_gui_update(struct autoboiler_data * autob_dat);
void initialize_controller( struct autoboiler_data * autob_dat);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LVGL_MYVIEW_H__*/
