/**
 * @file pwm.h
 * @brief Provides control for BBG's PWM channels.
 * @author Spencer Leslie 301571329.
 */
#ifndef SPENNYDL_PWM_H
#define SPENNYDL_PWM_H

#include <stdbool.h>

/** Status for OK. */
#define PWM_EOK 1
/** Status code indicating invalid argument. */
#define PWM_EBADARG -1
/** Status code indicating config-pin failed. */
#define PWM_ECONFIG -2
/** Status code indicating open() failed. */
#define PWM_EOPEN -3
/** Status code indicating write() failed. */
#define PWM_EWRITE -4
/** Status code indicating lseek() failed. */
#define PWM_ESEEK -5

/** Available PWM channels. */
typedef enum
{
    PWM_CHANNEL0A = 0,
    PWM_CHANNEL0B,
    PWM_CHANNEL1A,
    PWM_CHANNEL1B,
    PWM_CHANNEL2A,
    PWM_CHANNEL2B,
    PWM_NCHANNELS
} pwm_channel;

/** External-facing PWM handle. */
typedef struct
{
    /**
     * PWM channel. Should be set by consumers before calling
     * @ref pwm_open.
     */
    pwm_channel channel;
    /** Internal PWM data. */
    void* __pwm;
} pwm;

/**
 * Opens a PWM channel.
 * @param channel @ref pwm handle that is filled in upon opening
 *        the channel.
 * @return int Status code representing result of operation.
 */
int
pwm_open(pwm* channel);

/**
 * Set the period of a PWM channel in hertz.
 * @param pwm The @ref pwm channel to update.
 * @param hz The new period in hertz.
 * @return bool Whether or not updating was successful. Check errno on failure.
 */
int
pwm_set_period(pwm* pwm, unsigned long hz);

/**
 * Set the duty cycle of a pwm channel.
 * @param pwm The @ref pwm channel to update.
 * @param duty_cycle The new duty cycle as a floating-point value between 0
 *        and 1. The channel will be configured such that it will be on for
 *        duty_cycle * (1.0 / period) s.
 * @return bool Whether or not the update was successful. Check errno on
 *         failure.
 */
int
pwm_set_duty_cycle(pwm* pwm, double duty_cycle);

/**
 * Enable or disable output from the PWM channel.
 * \param pwm The \ref pwm channel to update.
 * \param enable Whether or not to enable output.
 * \return bool Whether or not the update was successful. Check errno
 *         on failure.
 */
int
pwm_enable(pwm* pwm, bool enable);

/**
 * Close a pwm channel.
 * @param pwm The @ref pwm channel to close.
 */
void
pwm_close(pwm* pwm);

#endif // SPENNYDL_PWM_H
