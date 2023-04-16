#include "myview_target.h"
#include <stdbool.h>
#include <pthread.h>
#include "lvgl_myview.h"
#ifdef SIMULATOR
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#else
//#include <sys/ioctl.h>
#include <fcntl.h>
#endif

static void SetRelayState(int fd, bool On)
{
    uint8_t buf = (uint8_t) (On ? 1 : 0);
    write(fd, &buf, sizeof(buf));
}

static void * controller_thread(void * arg)
{
    struct autoboiler_data * priv = arg;
    const char * relayfilepath = FILEPATH_RELAY;

    int fd_relay = open(relayfilepath, O_WRONLY);
    if(fd_relay < 0 )
    {
        gerr("ERROR: failed to initialize file descriptor");
    }
    for(;;)
    {
        if(priv->temp_probe >= priv->target_temp)
        {
            priv->relay_on = false;
        }
        SetRelayState(fd_relay, priv->relay_on);
        usleep(30000);
    }
    return NULL;
}

void initialize_controller( struct autoboiler_data * autob_dat)
{
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, controller_thread, (void *)autob_dat);
    if (ret != OK)
    {
        gerr("ERROR: failed to initialize controller pthread\n");
    }
}