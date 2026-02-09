#include <string.h>
#include <curl/curl.h>
#include <pthread.h>

#include "compat.h"

#ifndef _WIN32
#include <signal.h>
#endif

#include "error.h"
#include "discord-worker.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int shutdown_fds[2] = {
    -1,
    -1,
};

static int init_counter = 0;

void
ccord_shutdown_async(void)
{
    char b = 0;
#ifdef _WIN32
    send((SOCKET)shutdown_fds[1], &b, sizeof b, 0);
#else
    write(shutdown_fds[1], &b, sizeof b);
#endif
}

int
ccord_shutting_down(void)
{
    struct pollfd pfd;
    memset(&pfd, 0, sizeof pfd);
    pfd.fd = shutdown_fds[0];
    pfd.events = POLLIN;
    if (-1 == shutdown_fds[0]) return 0;
    poll(&pfd, 1, 0);
    return !!(pfd.revents & POLLIN);
}

#ifdef CCORD_SIGINTCATCH
/* shutdown gracefully on SIGINT received */
#ifdef _WIN32
static BOOL WINAPI
_ccord_console_handler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        const char err_str[] =
            "\nSIGINT: Disconnecting running concord client(s) ...\n";
        _write(2, err_str, (unsigned)strlen(err_str));
        ccord_shutdown_async();
        return TRUE;
    }
    return FALSE;
}
#else
static void
_ccord_sigint_handler(int signum)
{
    (void)signum;
    const char err_str[] =
        "\nSIGINT: Disconnecting running concord client(s) ...\n";
    write(STDERR_FILENO, err_str, strlen(err_str));
    ccord_shutdown_async();
}
#endif /* _WIN32 */
#endif /* CCORD_SIGINTCATCH */

CCORDcode
ccord_global_init()
{
    pthread_mutex_lock(&lock);
    if (0 == init_counter++) {
#ifdef CCORD_SIGINTCATCH
#ifdef _WIN32
        SetConsoleCtrlHandler(_ccord_console_handler, TRUE);
#else
        signal(SIGINT, &_ccord_sigint_handler);
#endif
#endif
        if (0 != curl_global_init(CURL_GLOBAL_DEFAULT)) {
            fputs("Couldn't start libcurl's globals\n", stderr);
            goto fail_curl_init;
        }
        if (0 != discord_worker_global_init()) {
            fputs("Attempt duplicate global initialization\n", stderr);
            goto fail_discord_worker_init;
        }
        if (0 != compat_socketpair(shutdown_fds)) {
            fputs("Failed to create shutdown pipe\n", stderr);
            goto fail_pipe_init;
        }
        for (int i = 0; i < 2; i++) {
            if (0 != compat_set_nonblocking(shutdown_fds[i])) {
                fputs("Failed to make shutdown pipe nonblocking\n", stderr);
                goto fail_pipe_init;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return CCORD_OK;

fail_pipe_init:
    for (int i = 0; i < 2; i++) {
        if (-1 != shutdown_fds[i]) {
            compat_close_socket(shutdown_fds[i]);
            shutdown_fds[i] = -1;
        }
    }
fail_discord_worker_init:
    discord_worker_global_cleanup();
fail_curl_init:
    curl_global_cleanup();

    init_counter = 0;
    pthread_mutex_unlock(&lock);
    return CCORD_GLOBAL_INIT;
}

void
ccord_global_cleanup()
{
    pthread_mutex_lock(&lock);
    if (init_counter && 0 == --init_counter) {
        curl_global_cleanup();
        discord_worker_global_cleanup();
        for (int i = 0; i < 2; i++) {
            compat_close_socket(shutdown_fds[i]);
            shutdown_fds[i] = -1;
        }
    }
    pthread_mutex_unlock(&lock);
}

int
discord_dup_shutdown_fd(void)
{
    if (-1 == shutdown_fds[0]) return -1;
#ifdef _WIN32
    /* On Windows we duplicate the socket via WSA */
    WSAPROTOCOL_INFOW info;
    if (WSADuplicateSocketW((SOCKET)shutdown_fds[0], GetCurrentProcessId(), &info) != 0)
        return -1;
    SOCKET s = WSASocketW(info.iAddressFamily, info.iSocketType, info.iProtocol, &info, 0, 0);
    if (s == INVALID_SOCKET) return -1;
    if (0 != compat_set_nonblocking((int)s)) {
        closesocket(s);
        return -1;
    }
    return (int)s;
#else
    int fd = -1;
    if (-1 != (fd = dup(shutdown_fds[0]))) {
        const int on = 1;

        #ifdef FIOCLEX
            if (0 != ioctl(fd, FIOCLEX, NULL)) {
                close(fd);
                return -1;
            }
        #endif

        if (0 != ioctl(fd, (int)FIONBIO, &on)) {
            close(fd);
            return -1;
        }
    }
    return fd;
#endif
}
