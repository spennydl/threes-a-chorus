/**
 * @file accel.h
 * @brief Interface to the Zen Cape's acclerometer.
 *
 * Allows access to the accelerometer either by reading directly or
 * polling for events.
 *
 * When polling for events, the @ref accell_get_events function must
 * be called once every 10ms. Calling it on shorter or longer intervals
 * will result in a shorter or longer debounce time and will affect the
 * reliability of event detection.
 *
 * @author Spencer Leslie 301571329
 *
 */
#pragma once

#include <stdint.h>

#define ACCEL_MAX_READING 2047
#define ACCEL_MIN_READING -2048

/**
 * @brief A sample from the accelerometer.
 *
 * Swizzled to allow either array access or
 * struct member access.
 *
 * Axis values will be in the range [-2048, 2047]. One G
 * corresponds to a reading of approximately 1024.
 */
typedef struct
{
    union
    {
        /** X Y and Z axis values as an array. */
        int16_t xzy[3];
        /** X Y and Z axis values as struct members. */
        struct
        {
            /** Left/Right. Negative for left, positive for right. */
            int16_t x;
            /** Forward/Back. Negative for forward, positive for back. */
            int16_t y;
            /**
             * Up/Down. Positive for down, negative for up.
             *
             * 1024 is subtracted from the raw accelerometer reading to
             * account for the constant force of gravity.
             */
            int16_t z;
        };
    };
} Accel_Sample;

/** Accelerometer axes. */
typedef enum
{
    ACCEL_X = 0,
    ACCEL_Y,
    ACCEL_Z,
    ACCEL_AXES
} Accel_Axis;

/**
 * @brief Initialize and set up the accelerometer.
 * @return 0 on success or an error code from the I2C module on failure.
 */
int
Accel_open(void);

/**
 * @brief Read a sample from the accelerometer.
 *
 * @param buf An @ref accel_sample_t to fill in with the read sample.
 * @return int 0 on success or a negative I2C error code on failure.
 */
int
Accel_read(Accel_Sample* buf);

/**
 * @brief Turn the accelerometer off and clean up any resources used.
 */
void
Accel_close(void);
