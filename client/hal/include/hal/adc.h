/**
 * @file adc.h
 * @brief Analog to Digital Converter functions.
 * @author Spencer Leslie 301571329
 */
#pragma once

/**
 * ADC error code. This value is chosen to be well outside of the valid range
 * of 0-4095.
 */
#define ADC_ERROR 8192
/**
 * Reference voltage of the ADC.
 */
#define ADC_REFERENCE_VOLTAGE 1.8
/**
 * Maximum possible raw reading.
 */
#define ADC_MAX_READING 4095

/**
 * Available ADC channels.
 */
typedef enum
{
    ADC_CHANNEL0 = 0,
    ADC_CHANNEL1,
    ADC_CHANNEL2,
    ADC_CHANNEL3,
    ADC_CHANNEL4,
    ADC_CHANNEL5,
    ADC_CHANNEL6
} adc_channel;

/**
 * Reads the current value of an ADC channel and returns the raw value.
 * @param @ref adc_channel The ADC channel to read.
 * @return unsigned int The raw value.
 */
unsigned int
adc_voltage_raw(adc_channel channel);

/**
 * Reads the current value of an ADC channel and returns the reading in volts.
 * @param @ref adc_channel The ADC channel to read.
 * @return double The value read in volts.
 */
double
adc_voltage(adc_channel channel);

/**
 * Converts a raw ADC reading to volts.
 * @param raw_value The raw reading.
 * @return double The converted reading in volts.
 */
double
adc_raw_to_voltage(double raw_value);
