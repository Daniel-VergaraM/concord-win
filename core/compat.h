/**
 * @file compat.h
 * @brief Cross-platform compatibility layer (Linux / macOS / Windows)
 *
 * Provides shims for POSIX APIs that are not available on Windows:
 *   - socketpair() via TCP loopback on 127.0.0.1
 *   - Non-blocking socket helpers
 *   - poll()  -> WSAPoll()
 *   - close() -> closesocket()
 *   - Various POSIX function aliases (_stricmp, _getpid, etc.)
 */
#ifndef COMPAT_H
#define COMPAT_H

#ifdef _WIN32

/* Must be included before <windows.h> to avoid fd_set redefinition */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>       /* _write, _read */
#include <process.h>  /* _getpid */

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")

/* ---- poll() ----------------------------------------------------------- */
#define poll       WSAPoll
#define POLLIN     POLLRDNORM
#define POLLOUT    POLLWRNORM

/* ---- POSIX string helpers --------------------------------------------- */
#define strcasecmp   _stricmp
#define strncasecmp  _strnicmp

/* ---- POSIX process helpers -------------------------------------------- */
#define getpid       _getpid

/* ---- close for sockets ------------------------------------------------ */
/* Only use compat_close_socket() for SOCKET handles.
 * Regular file descriptors still use _close(). */
#define compat_close_socket closesocket

/* ---- socketpair() emulation over TCP loopback ------------------------- */
static inline int
compat_socketpair(int fds[2])
{
    SOCKET listener = INVALID_SOCKET;
    SOCKET client   = INVALID_SOCKET;
    SOCKET server   = INVALID_SOCKET;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) goto fail;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = 0; /* Let OS pick a port */

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
        goto fail;
    if (getsockname(listener, (struct sockaddr *)&addr, &addrlen) == SOCKET_ERROR)
        goto fail;
    if (listen(listener, 1) == SOCKET_ERROR)
        goto fail;

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET) goto fail;
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
        goto fail;

    server = accept(listener, NULL, NULL);
    if (server == INVALID_SOCKET) goto fail;

    closesocket(listener);

    fds[0] = (int)server;
    fds[1] = (int)client;
    return 0;

fail:
    if (listener != INVALID_SOCKET) closesocket(listener);
    if (client   != INVALID_SOCKET) closesocket(client);
    if (server   != INVALID_SOCKET) closesocket(server);
    return -1;
}

/* ---- set fd/socket non-blocking --------------------------------------- */
static inline int
compat_set_nonblocking(int fd)
{
    u_long mode = 1;
    return ioctlsocket((SOCKET)fd, FIONBIO, &mode);
}

/* ---- Winsock startup / cleanup ---------------------------------------- */
static inline void
compat_winsock_init(void)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

static inline void
compat_winsock_cleanup(void)
{
    WSACleanup();
}

#else /* POSIX (Linux / macOS) */

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <strings.h>
#include <sys/ioctl.h>

#define compat_close_socket close

static inline int
compat_socketpair(int fds[2])
{
    /* On POSIX we just use pipe() – lighter weight */
    return pipe(fds);
}

static inline int
compat_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static inline void compat_winsock_init(void) { }
static inline void compat_winsock_cleanup(void) { }

#endif /* _WIN32 */

#endif /* COMPAT_H */
