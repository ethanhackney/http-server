#include "../include/util.h"
#include <errno.h>
#include <string.h>

void
do_die(const char *file, const char *func, int line, const char *fmt, ...)
{
        va_list va;
        int tmp = -1;
        int err = -1;

        tmp = errno;
        err = STDERR_FILENO;
        dprintf(err, "[%d:%s:%s:%d]: ", (int)getpid(), file, func, line);
        va_start(va, fmt);
        vdprintf(err, fmt, va);
        va_end(va);
        dprintf(err, ": %s\n", strerror(tmp));
        _exit(EXIT_FAILURE);
}

void
do_die_no_errno(const char *file, const char *func, int line, const char *fmt, ...)
{
        va_list va;
        int err = -1;

        err = STDERR_FILENO;
        dprintf(err, "[%d:%s:%s:%d]: ", (int)getpid(), file, func, line);
        va_start(va, fmt);
        vdprintf(err, fmt, va);
        va_end(va);
        dprintf(err, "\n");
        _exit(EXIT_FAILURE);
}
