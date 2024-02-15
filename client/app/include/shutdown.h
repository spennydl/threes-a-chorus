/**
 * @file shutdown.h
 * @author Spencer Leslie 301571329.
 * @brief Listens to events and notifies when shutdown should occur.
 *
 * The Shutdown module listens for events that would trigger shutdown and
 * notifies through a callback when a shutdown has been requested.
 *
 * Shutdown events occur when:
 * - The process receives SIGINT
 * - Any input is received on STDIN (i.e. the user presses enter)
 * - The @ref shutdown_request_shutdown function is called
 */
#ifndef SPENNYDL_SHUTDOWN_H
#define SPENNYDL_SHUTDOWN_H

/** OK status code. */
#define SHUTDOWN_CONTINUE 0
/** Status indicating shutdown has been requested. */
#define SHUTDOWN_REQUESTED 1
/** Error code indicating failure to install SIGINT handler. */
#define SHUTDOWN_ESIGNAL -1
/** Error code indicating failure to start the listener thread. */
#define SHUTDOWN_ETHREAD -2
/**
 * Error code indicating the function passed to
 * @ref shutdown_install was NULL.
 */
#define SHUTDOWN_EINVAL -2

/**
 * Type of the notify function.
 */
typedef void
shutdown_notify_fn(int);

/**
 * @brief Installs the shutdown listeners and begins listening for shutdown
 * requests.
 *
 * When a shutdown is requested, the @ref shutdown_notify_fn callback will be
 * invoked with the current shutdown status, which should be SHUTDOWN_REQUESTED.
 *
 * @param notify_fn The callback to call when shutdown is requested.
 * @return int SHUTDOWN_CONTINUE on success, and a negative number on failure.
 */
int
shutdown_install(shutdown_notify_fn* notify_fn);

/**
 * @brief Requests application shutdown.
 */
void
shutdown_request_shutdown(void);

#endif /* SPENNYDL_SHUTDOWN_H */
