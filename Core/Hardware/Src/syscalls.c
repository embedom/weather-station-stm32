/**
 ******************************************************************************
 * @file        : syscalls.c
 * @author      : none
 * @date        : 2026-04-12
 * @brief       : Minimal newlib syscall stubs for bare-metal ARM targets.
 * 
 * These stubs satisfy the linker when using newlib/newlib-nano without an OS 
 * providing POSIX I/O. They silence the "is not implemented" linker warnings.
 * 
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <sys/stat.h>
#include <errno.h>

/********************************* FUNCTIONS *********************************/

int _close(int fd)
{
    (void)fd;
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

int _lseek(int fd, int offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return -1;
}

int _read(int fd, char *buf, int len)
{
    (void)fd;
    (void)buf;
    (void)len;
    return 0;
}

int _write(int fd, const char *buf, int len)
{
    (void)fd;
    (void)buf;
    return len;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}
