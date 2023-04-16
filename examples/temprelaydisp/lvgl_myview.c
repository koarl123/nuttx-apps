

#include "myview_target.h"
//#include "../../lv_demo.h"
#ifdef SIMULATOR
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#endif

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "lvgl_myview.h"

static struct autoboiler_data g_autoboiler_data =
{
    .relay_on = false,
    .temp_env = 0,
    .temp_probe = 0
};

/* initialize my mega view */
void lv_myview(void)
{

#ifdef SIMULATOR
    myview_sim_initialize_sensors();
#endif

    initialize_controller(&g_autoboiler_data);
    initialize_gui_update(&g_autoboiler_data);
    initialize_data_collection(&g_autoboiler_data);
}