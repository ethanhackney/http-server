#ifndef LIB_H
#define LIB_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * print message + errno message and exit:
 *
 * args:
 *  @file: file
 *  @func: function
 *  @line: line
 *  @fmt:  format string
 *  @...:  arguments
 *
 * ret:
 *  exit process
 */
void do_die(const char *file, const char *func, int line, const char *fmt, ...);

/**
 * print message + errno message and exit:
 *
 * args:
 *  @_fmt: format string
 *  @...:  arguments
 *
 * ret:
 *  exit process
 */
#define die(_fmt, ...) \
        do_die(__FILE__, __func__, __LINE__, _fmt, ##__VA_ARGS__)

/* if debugging */
#ifdef DBUG
/**
 * test condition and if true, print message and exit:
 *
 * args:
 *  @cond: condition to test
 *  @file: name of file
 *  @func: name of function
 *  @line: line number
 *  @fmt:  format string
 *  @...:  arguments
 *
 * ret:
 *  exit process
 */
static inline void
do_dbug(bool cond,
        const char *file,
        const char *func,
        int line,
        const char *fmt, ...)
{
        va_list va;

        if (!cond)
                return;

        fprintf(stderr, "[%s:%s:%d]: ", file, func, line);
        va_start(va, fmt);
        vfprintf(stderr, fmt, va);
        va_end(va);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
}
#else
static inline void
do_dbug(bool cond,
        const char *file,
        const char *func,
        int line,
        const char *fmt, ...)
{
}
#endif /* #ifdef DBUG */

/**
 * test condition and if true, print message and exit:
 *
 * args:
 *  @_cond: condition to test
 *  @_fmt:  format string
 *  @...:   arguments
 *
 * ret:
 *  exit process
 */
#define dbug(_cond, _fmt, ...) \
        do_dbug(_cond, __FILE__, __func__, __LINE__, _fmt, ##__VA_ARGS__)

#endif /* #ifndef LIB_H */
