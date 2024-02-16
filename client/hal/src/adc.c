/**
 * @file adc.c
 * @brief Implementation of the functions defined in @ref adc.h.
 * @author Spencer Leslie 301571329.
 */
#include "hal/adc.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

/** Format string for the files representing ADC channels. */
#define ADC_SYSFS_FILE_FMT "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw"

#define ADC_BUFSIZ 64

unsigned int
adc_voltage_raw(adc_channel channel)
{
    assert((channel >= ADC_CHANNEL0 && channel <= ADC_CHANNEL6));

    char filename[ADC_BUFSIZ];
    snprintf(filename, ADC_BUFSIZ, ADC_SYSFS_FILE_FMT, channel);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return ADC_ERROR;
    }
    char buf[ADC_BUFSIZ];
    int bytes_read = read(fd, buf, ADC_BUFSIZ - 1);

    if (bytes_read <= 0) {
        return ADC_ERROR;
    }
    close(fd);

    buf[bytes_read] = '\0';
    unsigned int ret = atoi(buf);
    return ret;
}

double
adc_voltage(adc_channel channel)
{
    unsigned int raw = adc_voltage_raw(channel);
    if (raw != ADC_ERROR) {
        return adc_raw_to_voltage(raw);
    }
    return ADC_ERROR;
}

double
adc_raw_to_voltage(double raw_value)
{
    return (raw_value * ADC_REFERENCE_VOLTAGE) / ADC_MAX_READING;
}
