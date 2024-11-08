
#define SIMULATOR

#ifdef SIMULATOR
#include "myview_simulator.h"
#include "debug.h"
#include <unistd.h>

#define FILEPATH_ADC "adc.fifo"
#define FILEPATH_GYRO "sensor_gyro_uncal0.fifo"
#define FILEPATH_RELAY "relay0.file"
#else
#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <sys/ioctl.h>
#include <debug.h>

#define FILEPATH_ADC "/dev/adc0"
#define FILEPATH_GYRO "/dev/uorb/sensor_gyro_uncal0"
#define FILEPATH_RELAY "/dev/gpio0"
#endif
