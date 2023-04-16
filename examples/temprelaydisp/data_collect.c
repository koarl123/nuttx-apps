#include "myview_target.h"
#ifdef SIMULATOR
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>
#include <nuttx/sensors/sensor.h>

#include <debug.h>
#endif
#include <sys/ioctl.h>

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "lvgl_myview.h"

#define ADC_FILTER_BUF_SIZE 20

#define PT1000TAB_TEMP_MIN -50
#define PT1000TAB_TEMP_MAX 259
#define PT1000TAB_ENTRIES 310
// table from -50 degree up to 259 degree celsius
static float Pt1000TemperatureTable[PT1000TAB_ENTRIES] =
    {  /*   0        1        2        3        4        5        6        7        8        9*/
/*-50*/ 803.1,  807.00,  811.00,  815.00,  818.90,  822.90,  826.90,  830.80,  834.80,  838.80,
       842.90,  846.70,  850.60,  854.60,  858.50,  862.50,  866.40,  870.40,  874.30,  878.30,
       822.20,  886.20,  890.10,  894.00,  898.00,  901.90,  905.90,  909.80,  913.70,  917.70,
       921.60,  925.50,  929.50,  933.40,  937.30,  941.20,  945.20,  949.10,  953.00,  956.90,
       960.90,  964.80,  968.70,  972.60,  976.50,  980.40,  984.40,  988.30,  992.20,  996.10,
         1000,  1003.9,  1007.8,  1011.7,  1015.6,  1019.5,  1023.4,  1027.3,  1031.2,  1035.1,
         1039,  1042.9,  1046.8,  1050.7,  1054.6,  1058.5,  1062.4,  1066.3,  1070.2,  1074,
       1077.9,  1081.8,  1085.7,  1089.6,  1093.5,  1097.3,  1101.2,  1105.1,    1109,  1112.8,
       1116.7,  1120.6,  1124.5,  1128.3,  1132.2,  1136.1,  1139.9,  1143.8,  1147.7,  1151.5,
       1155.4,  1159.3,  1163.1,  1167,    1170.8,  1174.7,  1178.5,  1182.4,  1186.2,  1190.1,
         1194,  1197.8,  1201.6,  1205.5,  1209.3,  1213.2,    1217,  1220.9,  1224.7,  1228.6,
       1232.4,  1236.2,  1240.1,  1243.9,  1247.7,  1251.6,  1255.4,  1259.2,  1263.1,  1266.9,
       1270.7,  1274.5,  1278.4,  1282.2,    1286,  1289.8,  1293.7,  1297.5,  1301.3,  1305.1,
       1308.9,  1312.7,  1316.6,  1320.4,  1324.2,    1328,  1331.8,  1335.6,  1339.4,  1343.2,
         1347,  1350.8,  1354.6,  1358.4,  1362.2,    1366,  1369.8,  1373.6,  1377.4,  1381.2,
         1385,  1388.8,  1392.6,  1396.4,  1400.2,  1403.9,  1407.7,  1411.5,  1415.3,  1419.1,
       1422.9,  1426.6,  1430.4,  1434.2,    1438,  1441.7,  1445.5,  1449.3,  1453.1,  1456.8,
       1460.6,  1464.4,  1468.1,  1471.9,  1475.7,  1479.4,  1483.2,    1487,  1490.7,  1494.5,
       1498.2,    1502,  1505.7,  1509.5,  1513.3,    1517,  1520.8,  1524.5,  1528.3,  1532,
       1535.8,  1539.5,  1543.2,    1547,  1550.7,  1554.5,  1558.2,  1561.9,  1565.7,  1569.4,
       1573.1,  1576.9,  1580.6,  1584.3,  1588.1,  1591.8,  1595.5,  1599.3,    1603,  1606.7,
       1610.4,  1614.2,  1617.9,  1621.6,  1625.3,    1629,  1632.7,  1636.5,  1640.2,  1643.9,
       1647.6,  1651.3,    1655,  1658.7,  1662.4,  1666.1,  1669.8,  1673.5,  1677.2,  1680.9,
       1684.6,  1688.3,    1692,  1695.7,  1699.4,  1703.1,  1706.8,  1710.5,  1714.2,  1711.9,
       1721.6,  1725.3,    1729,  1732.6,  1736.3,    1740,  1743.7,  1747.4,    1751,  1754.7,
       1758.4,  1762.1,  1765.7,  1769.4,  1773.1,  1776.8,  1780.4,  1784.1,  1787.8,  1791.4,
       1795.1,  1798.8,  1802.4,  1806.1,  1809.7,  1813.4,  1817.1,  1820.7,  1824.4,  1828,
       1831.7,  1835.3,    1839,  1842.6,  1846.3,  1849.9,  1853.6,  1857.2,  1860.9,  1864.5,
       1868.2,  1871.8,  1875.4,  1879.1,  1882.7,  1886.3,    1890,  1893.6,  1897.2,  1900.9,
       1904.5,  1908.1,  1911.8,  1915.4,    1919,  1922.6,  1926.3,  1929.9,  1933.5,  1937.1,
       1940.7,  1944.4,    1948,  1951.6,  1955.2,  1958.8,  1962.4,    1966,  1969.6,  1973.3,
};


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

static int GetFilteredTempFromAdc(int adc_data_raw)
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
    struct autoboiler_data *priv = (struct autoboiler_data *)arg;
    if(priv==NULL)
    {
        gerr("error in thread __FUNCTION__\n");
        return NULL;
    }
    const char *adcfilepath = FILEPATH_ADC;
    const char *gyrofilepath = FILEPATH_GYRO;

#ifdef SIMULATOR
    mkfifo(gyrofilepath, 0666);
    mkfifo(adcfilepath, 0666);
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
#ifndef SIMULATOR
        // now get ADC data
        int ret = ioctl(fd_adc, ANIOC_TRIGGER, 0);
        if (ret < 0)
        {
            int errcode = errno;
            gwarn("adc ANIOC_TRIGGER ioctl failed: %d\n", errcode);
        }
        else
#endif
        {
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

void initialize_data_collection( struct autoboiler_data *autob_dat)
{
    pthread_t thread;
    /* start thread for data collection from sensors */
    int ret = pthread_create(&thread, NULL, data_collector_thread, (void *)autob_dat);
    if (ret != OK)
    {
        gerr("ERROR: failed to initialize data collector pthread\n");
    }
}