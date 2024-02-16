#include "shutdown.h"

#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

/** Timeout for the poll() on stdin. */
#define SHUTDOWN_STDINPOLL_TIMEOUT_MS 500
/** Buffer size for the buffer used to consume stdin. */
#define SHUTDOWN_STDINPOLL_BUFSIZ 128

/** Shutdown status flag. Requests to shutdown will set this to
 * SHUTDOWN_REQUESTED. */
static volatile sig_atomic_t _shutdown_status = SHUTDOWN_CONTINUE;
/** Thread handle for watcher thread. */
static pthread_t _shutdown_watch_thread;
/** The callback we will invoke when shutdown has been requested. */
static shutdown_notify_fn* _notify_fn;

/**
 * @brief SIGINT handler.
 *
 * Simply sets @ref _shutdown_status = SHUTDOWN_REQUESTED.
 *
 * @param sig The signal we are handling.
 */
static void
_shutdown_handle_sigint(int sig);

/**
 * @brief Installs the SIGINT handler.
 */
static int
_app_install_sigint_handler(void);

/**
 * @brief Shutdown listener worker thread function.
 *
 * Polls stdin for input and monitors @ref _shutdown_status for changes from
 * other sources.
 *
 * @param _arg_unused Unused thread argument.
 * @return void* Unusued thread return data.
 */
static void*
_shutdown_listener(void* _arg_unused);

static void
_shutdown_handle_sigint(int sig)
{
    (void)sig; // Only handling SIGINT, so this should be irrelevant.
    _shutdown_status = SHUTDOWN_REQUESTED;
}

static int
_app_install_sigint_handler(void)
{
    struct sigaction sigact;
    // setting SA_RESTART ensures that any interrupted system calls are
    // restarted. This is necessary if the application wants to gracefully
    // finish its current work item before shutting down.
    // TODO this could be configurable, but for this application we'll
    // leave it hardcoded.
    sigact.sa_flags = SA_RESTART;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigact.sa_handler = _shutdown_handle_sigint;

    return sigaction(SIGINT, &sigact, NULL);
}

static void*
_shutdown_listener(void* _arg_unused)
{
    (void)_arg_unused;

    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };
    while (_shutdown_status == SHUTDOWN_CONTINUE) {
        int poll_status;
        if ((poll_status = poll(&stdinp, 1, SHUTDOWN_STDINPOLL_TIMEOUT_MS)) <
            0) {
            // I'm not sure this is likely to happen. If it does we'll simply
            // continue.
            perror("SHUTDOWN: ERR: Polling stdin failed");
            continue;
        }
        if (poll_status > 0) {
            if ((stdinp.revents & (POLLIN | POLLPRI)) > 0) {
                // Read the line sent from stdin, otherwise it will get sent
                // to the shell after we exit.
                char buf[SHUTDOWN_STDINPOLL_BUFSIZ];
                fgets(buf, SHUTDOWN_STDINPOLL_BUFSIZ, stdin);

                _shutdown_status = SHUTDOWN_REQUESTED;
            }
        }
    }
    _notify_fn(_shutdown_status);
    return NULL;
}

int
shutdown_install(shutdown_notify_fn* notify)
{
    if (notify == NULL) {
        return SHUTDOWN_EINVAL;
    }
    _notify_fn = notify;

    if (pthread_create(&_shutdown_watch_thread, 0, _shutdown_listener, NULL) <
        0) {
        return SHUTDOWN_ETHREAD;
    }
    pthread_detach(_shutdown_watch_thread);

    if (_app_install_sigint_handler() < 0) {
        _shutdown_status = SHUTDOWN_REQUESTED;
        return SHUTDOWN_ESIGNAL;
    }

    return SHUTDOWN_CONTINUE;
}

void
shutdown_request_shutdown(void)
{
    _shutdown_status = SHUTDOWN_REQUESTED;
}
