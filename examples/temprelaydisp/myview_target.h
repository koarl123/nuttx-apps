
//#define SIMULATOR 1

#ifdef SIMULATOR
#include "myview_simulator.h"
#include <unistd.h>

#define FILEPATH_ADC "/tmp/adc0"
#define FILEPATH_GYRO "/tmp/sensor_gyro_uncal0"
#define FILEPATH_RELAY "/tmp/relay0"
#else
#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <sys/ioctl.h>
#include <debug.h>

#define FILEPATH_ADC "/dev/adc0"
#define FILEPATH_GYRO "/dev/uorb/sensor_gyro_uncal0"
#define FILEPATH_RELAY "/dev/gpio0"
#endif