#include "../include/util.h"
#include <errno.h>
#include <string.h>

void
do_die(const char *file, const char *func, int line, const char *fmt, ...)
{
        va_list va;
        int tmp = 0;

        tmp = errno;
        fprintf(stderr, "[%s:%s:%d]: ", file, func, line);
        va_start(va, fmt);
        vfprintf(stderr, fmt, va);
        va_end(va);
        fprintf(stderr, ": %s\n", strerror(tmp));
        exit(EXIT_FAILURE);
}
