
#include <stdint.h>

void myview_sim_initialize_sensors(void);

struct adc_msg_s
{
    uint32_t am_data;
};

struct sensor_gyro
{
    uint32_t temperature;
};