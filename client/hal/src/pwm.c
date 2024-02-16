/**
 * @file pwm.c
 * @brief Implementation of the functions defined in pwm.h.
 * @author Spencer Leslie 301571329
 */
#include "hal/pwm.h"
#include "hal/fileio.h"

#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/** Format strings for the files representing PWM channels. */
#define PWM_0A_DIRECTORY_FMT "/dev/bone/pwm/0/a/%s"
#define PWM_0B_DIRECTORY_FMT "/dev/bone/pwm/0/b/%s"
#define PWM_1A_DIRECTORY_FMT "/dev/bone/pwm/1/a/%s"
#define PWM_1B_DIRECTORY_FMT "/dev/bone/pwm/1/b/%s"
#define PWM_2A_DIRECTORY_FMT "/dev/bone/pwm/2/a/%s"
#define PWM_2B_DIRECTORY_FMT "/dev/bone/pwm/2/b/%s"

/** File names of the devfs pwm interface functions of interest. */
#define PWM_PERIOD_FILE "period"
#define PWM_DUTYCYCLE_FILE "duty_cycle"
#define PWM_ENABLE_FILE "enable"

/** PWM config-pin command format string. */
#define PWM_CONFIGPIN_CMD_FMT "config-pin P9_%d pwm"

/** Nanoseconds in a second. */
#define NS_IN_SECOND 1000000000

/** Internal PWM data. */
typedef struct
{
    /** File descriptor for period file.*/
    int period_fd;
    /** File descriptor for duty cycle file.*/
    int duty_cycle_fd;
    /** File descriptor for enable file.*/
    int enable_fd;
    /** Currently configured period in nanoseconds. */
    unsigned int current_period_ns;
    /** Currently configured duty cycle. */
    double current_duty_cycle;
} _pwm;

/** Convenience struct for const data organization. */
typedef struct
{
    const int pin;
    const char* directory;
} _pwm_data;

/** Table of const values for the available PWM channels. */
static const _pwm_data PWM_CHANNELS[PWM_NCHANNELS] = {
    { .pin = 22, .directory = PWM_0A_DIRECTORY_FMT },
    { .pin = 21, .directory = PWM_0B_DIRECTORY_FMT },
    { .pin = 14, .directory = PWM_1A_DIRECTORY_FMT },
    { .pin = 16, .directory = PWM_1B_DIRECTORY_FMT },
    { .pin = 19, .directory = PWM_2A_DIRECTORY_FMT },
    { .pin = 13, .directory = PWM_2B_DIRECTORY_FMT }
};

/**
 * Allocates a new @ref _pwm.
 * @return The allocated @ref _pwm, or NULL on failure.
 */
static _pwm*
_pwm_new(void);

/**
 * Runs `config-pin` to configure the correct pins for the
 * given @ref pwm_channel.
 * @param c The @ref pwm_channel to configure.
 * @return bool Whether or not configuration succeeded.
 */
static bool
_pwm_config_pin(pwm_channel c);

/**
 * Performs the actual update of the duty cycle value.
 * @param duty_cycle_fd File descriptor of the duty cycle file.
 * @param period_ns The current period.
 * @param duty_cycle The new duty cycle.
 * @return int The return status as given by write().
 */
static int
_pwm_update_duty_cycle(int duty_cycle_fd,
                       unsigned long period_ns,
                       double duty_cycle);

/**
 * Performs the actual update of the period.
 * @param period_fd File descriptor of the period file.
 * @param period_ns The new period.
 * @return int The return status as given by write().
 */
static int
_pwm_update_period(int period_fd, unsigned long period_ns);

static _pwm*
_pwm_new(void)
{
    return malloc(sizeof(_pwm));
}

static bool
_pwm_config_pin(pwm_channel c)
{
    char cmd[64];
    snprintf(cmd, 64, PWM_CONFIGPIN_CMD_FMT, PWM_CHANNELS[c].pin);

    int cmd_exit = fileio_pipe_command(cmd);
    return cmd_exit == 0;
}

static int
_pwm_update_duty_cycle(int duty_cycle_fd,
                       unsigned long period_ns,
                       double duty_cycle)
{
    unsigned long ns = period_ns * duty_cycle;

    char buf[32];
    int chars = snprintf(buf, 32, "%lu", ns);
    int written = write(duty_cycle_fd, buf, chars);
    // lseek(duty_cycle_fd, 0, SEEK_SET);

    return written;
}

static int
_pwm_update_period(int period_fd, unsigned long period_ns)
{
    char buf[32];

    int chars = snprintf(buf, 32, "%lu", period_ns);
    int written = write(period_fd, buf, chars);
    // lseek(period_fd, 0, SEEK_SET);

    return written;
}

int
pwm_open(pwm* channel)
{
    if (!channel) {
        return PWM_EBADARG;
    }

    if (!_pwm_config_pin(channel->channel)) {
        fprintf(stderr, "PWM: WARN: Configpin failed\n");
    }

    char buf[64];
    snprintf(
      buf, 64, PWM_CHANNELS[channel->channel].directory, PWM_PERIOD_FILE);
    int period_fd = open(buf, O_WRONLY);
    if (period_fd < 0) {
        return PWM_EOPEN;
    }

    snprintf(
      buf, 64, PWM_CHANNELS[channel->channel].directory, PWM_DUTYCYCLE_FILE);
    int duty_cycle_fd = open(buf, O_WRONLY);
    if (duty_cycle_fd < 0) {
        close(period_fd);
        return PWM_EOPEN;
    }

    snprintf(
      buf, 64, PWM_CHANNELS[channel->channel].directory, PWM_ENABLE_FILE);
    int enable_fd = open(buf, O_WRONLY);
    if (enable_fd < 0) {
        close(period_fd);
        close(duty_cycle_fd);
        return PWM_EOPEN;
    }

    _pwm* __pwm = _pwm_new();
    __pwm->period_fd = period_fd;
    __pwm->duty_cycle_fd = duty_cycle_fd;
    __pwm->enable_fd = enable_fd;
    // defaults
    __pwm->current_period_ns = 100000;
    __pwm->current_duty_cycle = 0.5;

    _pwm_update_duty_cycle(__pwm->period_fd, __pwm->current_period_ns, 0);
    _pwm_update_period(__pwm->period_fd, __pwm->current_period_ns);
    _pwm_update_duty_cycle(
      __pwm->period_fd, __pwm->current_period_ns, __pwm->current_duty_cycle);

    channel->__pwm = __pwm;
    return PWM_EOK;
}

int
pwm_set_period(pwm* p, unsigned long hz)
{
    if (!p || !p->__pwm) {
        return PWM_EBADARG;
    }
    _pwm* pwm = p->__pwm;
    unsigned long period_ns = hz == 0 ? 0 : NS_IN_SECOND / hz;

    // Do nothing if there is no change to be made
    if (pwm->current_period_ns == period_ns) {
        return PWM_EOK;
    }

    // Zero the duty cycle to prevent invalid updates
    int written = _pwm_update_duty_cycle(pwm->duty_cycle_fd, period_ns, 0);
    if (written < 0) {
        return PWM_EWRITE;
    }

    // Now we can update the period.
    if (period_ns > 0) {
        written = _pwm_update_period(pwm->period_fd, period_ns);
        if (written < 0) {
            return PWM_EWRITE;
        }
        pwm->current_period_ns = period_ns;
    }

    // and then the new duty cycle
    written = _pwm_update_duty_cycle(
      pwm->duty_cycle_fd, period_ns, pwm->current_duty_cycle);
    if (written < 0) {
        return PWM_EWRITE;
    }
    return PWM_EOK;
}

int
pwm_set_duty_cycle(pwm* p, double duty_cycle)
{
    if (!p || !p->__pwm) {
        return PWM_EBADARG;
    }
    _pwm* pwm = p->__pwm;
    // Do nothing if we don't need to update
    if (pwm->current_duty_cycle == duty_cycle) {
        return PWM_EOK;
    }

    unsigned long ns = round(pwm->current_period_ns * duty_cycle);

    int written =
      _pwm_update_duty_cycle(pwm->duty_cycle_fd, pwm->current_period_ns, ns);
    if (written < 0) {
        return PWM_EWRITE;
    }

    pwm->current_duty_cycle = duty_cycle;
    return PWM_EOK;
}

int
pwm_enable(pwm* p, bool enable)
{
    if (!p || !p->__pwm) {
        return PWM_EBADARG;
    }

    _pwm* pwm = p->__pwm;
    const char* val = enable ? "1" : "0";

    int status = write(pwm->enable_fd, val, 1);
    if (status < 0) {
        return PWM_EWRITE;
    }

    // if (lseek(pwm->enable_fd, 0, SEEK_SET) < 0) {
    // return PWM_ESEEK;
    //}
    return PWM_EOK;
}

void
pwm_close(pwm* p)
{
    if (p && p->__pwm) {
        // Turn the lights off when you leave.
        pwm_enable(p, false);

        _pwm* pwm = p->__pwm;
        close(pwm->period_fd);
        close(pwm->duty_cycle_fd);
        close(pwm->enable_fd);
        pwm = NULL;

        free(p->__pwm);
        p->__pwm = NULL;
    }
}
