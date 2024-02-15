/**
 * @file gpio.c
 * @brief Implementation of the GPIO module decared in gpio.h.
 * @author Spencer Leslie 301571329
 */
#include "hal/gpio.h"
#include "hal/fileio.h"

#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

/** GPIO sysfs files */
#define GPIO_SYSFS_EXPORT "/sys/class/gpio/export"
#define GPIO_SYSFS_GPIO_DIR_FMT "/sys/class/gpio/gpio%d"
#define GPIO_SYSFS_GPIO_PIN_DIRECTION_FMT "/sys/class/gpio/gpio%d/direction"
#define GPIO_SYSFS_GPIO_VALUE_FMT "/sys/class/gpio/gpio%d/value"
#define GPIO_SYSFS_GPIO_ACTIVELOW_FMT "/sys/class/gpio/gpio%d/active_low"
#define GPIO_SYSFS_GPIO_EDGE_FMT "/sys/class/gpio/gpio%d/edge"

/** GPIO config-pin command format string */
#define GPIO_CONFIGPIN_CMD_FMT "config-pin p%d.%d gpio"

/** GPIO directions */
#define GPIO_IN_DIRECTION "in"
#define GPIO_OUT_DIRECTION "out"

/** Internal GPIO pin data. */
typedef struct
{
    int fd;
} _gpio;

/**
 * Allocates a new gpio_pin.
 * @return gpio_pin* Pointer to the allocated gpio_pin or NULL on failure.
 */
static _gpio*
_gpio_new(void);

/**
 * Checks whether a GPIO pin has been exported and exports it if necessary.
 * @param linux_pin_number The linux pin number of the GPIO pin.
 * @return bool Whether or not the export succeeded.
 */
static int
_gpio_ensure_exported(unsigned char linux_pin_number);

/**
 * Calls config-pin to set up a pin to be used for GPIO.
 * @param bbg_header The header on the BBG that contains the desired pin.
 * @param bbg_pin_number The BBG number of the pin.
 * @return bool Whether or not the setup succeeded.
 */
static int
_gpio_config_pin_for_gpio(unsigned char bbg_header,
                          unsigned char bbg_pin_number);

/**
 * Opens a GPIO pin for input or output.
 * @param linux_pinno The Linux pin number of the desired pin.
 * @param direction The direction, input or output.
 * @return int A file descriptor that maps to the desired GPIO pin, or a
 *             negative number if an error occurred.
 */
static int
_gpio_open(unsigned char linux_pinno, gpio_pin_direction direction);

/**
 * Configure a GPIO pin to be triggered on both rising and falling edges
 * when using edge-triggered IO.
 * @param linux_pinno The linux pin number of the pin to configure.
 * @return bool Whether or not the configuration succeeded.
 */
static int
_gpio_configure_edge(unsigned char linux_pinno);

static _gpio*
_gpio_new(void)
{
    _gpio* pin = (_gpio*)malloc(sizeof(_gpio));
    return pin;
}

static int
_gpio_ensure_exported(unsigned char linux_pin_number)
{
    char gpio_dir[FILEIO_SMALL_BUFSIZE];
    snprintf(gpio_dir,
             FILEIO_SMALL_BUFSIZE,
             GPIO_SYSFS_GPIO_DIR_FMT,
             linux_pin_number);

    // If exported, the /sys/class/gpio/gpioN directory will
    // exist. Check for it, and export if need be.
    int dir_exists = fileio_directory_exists(gpio_dir);
    if (!dir_exists) {
        char pinno[FILEIO_SMALL_BUFSIZE];
        snprintf(pinno, FILEIO_SMALL_BUFSIZE, "%d", linux_pin_number);
        if (fileio_write(GPIO_SYSFS_EXPORT, pinno) < 0) {
            return GPIO_EWRITE;
        }
    } else if (dir_exists < 0) {
        return GPIO_EDIR;
    }
    return GPIO_EOK;
}

static int
_gpio_config_pin_for_gpio(unsigned char bbg_header,
                          unsigned char bbg_pin_number)
{
    char command[FILEIO_SMALL_BUFSIZE];
    snprintf(command,
             FILEIO_SMALL_BUFSIZE,
             GPIO_CONFIGPIN_CMD_FMT,
             bbg_header,
             bbg_pin_number);

    int exitCode = fileio_pipe_command(command);
    if (exitCode != 0) {
        return GPIO_ECONFIG;
    }
    return GPIO_EOK;
}

static int
_gpio_open(unsigned char linux_pinno, gpio_pin_direction direction)
{
    char gpio_val[FILEIO_SMALL_BUFSIZE];
    snprintf(
      gpio_val, FILEIO_SMALL_BUFSIZE, GPIO_SYSFS_GPIO_VALUE_FMT, linux_pinno);
    int gpiofd;
    // TODO: epoll(7) recommends O_NONBLOCK, but it doesn't seem
    // to impact it here. Would that affect our other reads/writes?
    if (direction == GPIO_IN) {
        gpiofd = open(gpio_val, O_RDONLY);
    } else {
        // We want effects to happen immediately upon write,
        // so set O_SYNC
        gpiofd = open(gpio_val, O_SYNC | O_WRONLY);
    }

    return gpiofd;
}

static int
_gpio_configure_edge(unsigned char linux_pinno)
{
    char gpio_edgef[FILEIO_SMALL_BUFSIZE];
    snprintf(
      gpio_edgef, FILEIO_SMALL_BUFSIZE, GPIO_SYSFS_GPIO_EDGE_FMT, linux_pinno);

    if (fileio_write(gpio_edgef, "both") < 0) {
        return GPIO_EWRITE;
    }
    return GPIO_EOK;
}

int
gpio_set_active_low(gpio* p, bool active_low)
{
    char active_low_f[FILEIO_SMALL_BUFSIZE];
    snprintf(active_low_f,
             FILEIO_SMALL_BUFSIZE,
             GPIO_SYSFS_GPIO_ACTIVELOW_FMT,
             p->linux_pin_number);

    char al_val[FILEIO_SMALL_BUFSIZE];
    snprintf(al_val, FILEIO_SMALL_BUFSIZE, "%c", active_low ? '1' : '0');

    if (fileio_write(active_low_f, al_val) < 0) {
        return GPIO_EWRITE;
    }
    return GPIO_EOK;
}

int
gpio_set_direction(gpio* p, gpio_pin_direction direction)
{
    char dir_f[FILEIO_SMALL_BUFSIZE];
    snprintf(dir_f,
             FILEIO_SMALL_BUFSIZE,
             GPIO_SYSFS_GPIO_PIN_DIRECTION_FMT,
             p->linux_pin_number);

    const char* dir =
      direction == GPIO_IN ? GPIO_IN_DIRECTION : GPIO_OUT_DIRECTION;

    if (fileio_write(dir_f, dir) < 0) {
        return GPIO_EWRITE;
    }

    p->direction = direction;
    return GPIO_EOK;
}

int
gpio_open(gpio* pin)
{
    // Configure the pin.
    int status;
    if ((status = _gpio_ensure_exported(pin->linux_pin_number)) < 0) {
        return status;
    }
    if ((status = _gpio_config_pin_for_gpio(pin->bbg_header,
                                            pin->bbg_pin_number)) < 0) {
        return status;
    }
    if ((status = _gpio_configure_edge(pin->linux_pin_number)) < 0) {
        return status;
    }
    if ((status = gpio_set_direction(pin, pin->direction)) < 0) {
        return status;
    }
    // Get a file descriptor mapped to the pin.
    int gpiofd = _gpio_open(pin->linux_pin_number, pin->direction);
    if (gpiofd < 0) {
        return GPIO_EOPEN;
    }

    _gpio* gpiopin = _gpio_new();
    gpiopin->fd = gpiofd;
    pin->__gpio = gpiopin;

    return GPIO_EOK;
}

int
gpio_read(gpio* p, int* out_value)
{
    _gpio* pin = p->__gpio;

    char val[FILEIO_SMALL_BUFSIZE];
    int nbytes = read(pin->fd, val, FILEIO_SMALL_BUFSIZE);

    if (nbytes < 0) {
        return GPIO_EREAD;
    } else if (nbytes == 0) { // No data available
        return GPIO_ENODATA;
    }
    *out_value = atoi(val);

    // restore the file pointer, otherwise subsequent
    // read()s will fail.
    if (lseek(pin->fd, 0, SEEK_SET) < 0) {
        return GPIO_ESEEK;
    }

    return GPIO_EOK;
}

int
gpio_wait(gpio* pins, size_t n_pins, int* out_vals, unsigned long timeout_ms)
{
    // This function was adapted from the epoll example code.
    int epollfd = epoll_create(1);

    // Register events that we wanna listen to for each of the n_pins pins
    for (size_t i = 0; i < n_pins; i++) {
        _gpio* p = pins[i].__gpio;
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET | EPOLLPRI;
        event.data.u32 = i;

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, p->fd, &event) < 0) {
            close(epollfd);
            return GPIO_ECTL;
        }
    }

    // Do the poll. Do it twice as we don't care about the initial one.
    // TODO Maybe we do care about the initial one? It would be nice if we
    // had a fn that could return both the initial state and the changed state
    // after an event is received. But we don't need that now.
    int n_ready;
    struct epoll_event events[4];
    for (int i = 0; i < 2; i++) {
        n_ready = epoll_wait(epollfd, events, n_pins, timeout_ms);
        if (n_ready < 0) {
            close(epollfd);
            return GPIO_EWAIT;
        }
    }
    close(epollfd);

    // Bail early if we've timed out.
    if (n_ready == 0) {
        return 0;
    }

    // Read the current state for any pin that changed.
    for (int i = 0; i < n_ready; i++) {
        size_t idx = events[i].data.u32;
        if (gpio_read(&pins[idx], &out_vals[idx]) < 0) {
            return GPIO_EREAD;
        }
    }

    return n_ready;
}

void
gpio_close(gpio* p)
{
    if (p->__gpio) {
        _gpio* pin = p->__gpio;
        //
        // turn the lights out
        if (p->direction == GPIO_OUT) {
            gpio_write(p, false);
        }

        close(pin->fd);
        free(pin);

        p->__gpio = NULL;
    }
}

int
gpio_write(gpio* p, bool value)
{
    _gpio* pin = p->__gpio;
    const char* val = value ? "1" : "0";

    if (write(pin->fd, val, 1) < 0) {
        return GPIO_EWRITE;
    }

    if (lseek(pin->fd, 0, SEEK_SET) < 0) {
        return GPIO_ESEEK;
    }

    return GPIO_EOK;
}
