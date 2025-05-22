#include "../../lib/include/util.h"
#include "../include/iobuf.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

/* if debugging */
#ifdef DBUG
/**
 * validate iobuf{} state:
 *
 * args:
 *  @_ip: pointer to iobuf{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define IOBUF_OK(_ip) do {                                              \
        bool _above = false;                                            \
        bool _below = false;                                            \
        bool _in_range = false;                                         \
                                                                        \
        dbug((_ip) == NULL, "ip == NULL");                              \
        dbug((_ip)->i_fd < 0, "ip->i_fd < 0");                          \
                                                                        \
        _above = (_ip)->i_in <= (_ip)->i_endp;                          \
        _below = (_ip)->i_endp <= (_ip)->i_in + IOBUF_SIZE;             \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "ip->i_endp not in ip->i_in");                 \
                                                                        \
        _above = (_ip)->i_in <= (_ip)->i_inp;                           \
        _below = (_ip)->i_inp <= (_ip)->i_endp;                         \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "ip->i_inp > ip->i_endp");                     \
                                                                        \
        _above = (_ip)->i_out <= (_ip)->i_outp;                         \
        _below = (_ip)->i_outp <= (_ip)->i_out + IOBUF_SIZE;            \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "ip->i_outp not in ip->i_out");                \
} while (0)
#else
#define IOBUF_OK(_ip) do {      \
        /* no-op */             \
} while (0)
#endif /* #ifdef DBUG */

/**
 * is input buffer empty:
 *
 * args:
 *  @ip: pointer to iobuf{}:
 *
 * ret:
 *  @true:  if input buffer empty
 *  @false: if not
 */
static int iobuf_empty(const struct iobuf *ip);

/**
 * fill input buffer:
 *
 * args:
 *  @ip: pointer to iobuf{}:
 *
 * ret:
 *  @success: 0 or IOBUF_EOF
 *  @failure: -1 and errno set
 */
static int iobuf_fill(struct iobuf *ip);

/**
 * is output buffer full:
 *
 * args:
 *  @ip: pointer to iobuf{}:
 *
 * ret:
 *  @true:  if output buffer full 
 *  @false: if not
 */
static int iobuf_full(const struct iobuf *ip);

int
iobuf_init(struct iobuf *ip, int fd)
{
        dbug(ip == NULL, "ip == NULL");
        dbug(fd < 0, "fd < 0");

        memset(ip, 0, sizeof(*ip));
        ip->i_inp = ip->i_in;
        ip->i_endp = ip->i_in;
        ip->i_outp = ip->i_out;
        ip->i_fd = fd;

        return 0;
}

int
iobuf_free(struct iobuf *ip)
{
        IOBUF_OK(ip);

        if (iobuf_flush(ip) < 0)
                return -1;

        memset(ip, 0, sizeof(*ip));
        ip->i_fd = -1;
        return 0;
}

int
iobuf_getc(struct iobuf *ip)
{
        int ret = -1;

        IOBUF_OK(ip);

        if (!iobuf_empty(ip))
                return *ip->i_inp++;

        ret = iobuf_fill(ip);
        IOBUF_OK(ip);
        if (ret == IOBUF_EOF)
                return ret;
        if (ret < 0)
                return ret;

        return *ip->i_inp++;
}

static int
iobuf_empty(const struct iobuf *ip)
{
        IOBUF_OK(ip);
        return ip->i_inp == ip->i_endp;
}

static int
iobuf_fill(struct iobuf *ip)
{
        ssize_t n = -1;

        IOBUF_OK(ip);
again:
        n = read(ip->i_fd, ip->i_in, IOBUF_SIZE);
        if (n < 0 && errno == EINTR)
                goto again;
        if (n < 0)
                return -1;
        if (n == 0)
                return IOBUF_EOF;

        ip->i_inp = ip->i_in;
        ip->i_endp = ip->i_in + n;
        IOBUF_OK(ip);
        return 0;
}

void
iobuf_move(struct iobuf *dst, struct iobuf *src)
{
        dbug(dst == NULL, "dst == NULL");
        IOBUF_OK(src);
        memcpy(dst, src, sizeof(*dst));
        memset(src, 0, sizeof(*src));
        src->i_fd = -1;
}

int
iobuf_printf(struct iobuf *ip, const char *fmt, ...)
{
        const char *end = NULL;
        const char *p = NULL;
        va_list va;
        size_t nleft = 0;
        size_t ncopy = 0;
        char buf[IOBUF_SIZE] = "";
        int n = -1;

        va_start(va, fmt);
        n = vsnprintf(buf, sizeof(buf), fmt, va);
        if (n < 0)
                return -1;
        va_end(va);

        p = buf;
        end = p + IOBUF_SIZE;
        nleft = (size_t)n;
        while (nleft > 0) {
                if (iobuf_full(ip)) {
                        if (iobuf_flush(ip) < 0)
                                return -1;
                }
                ncopy = min(nleft, (size_t)(end - ip->i_outp));
                memcpy(ip->i_outp, p, ncopy);
                ip->i_outp += ncopy;
                p += ncopy;
                nleft -= ncopy;
        }

        IOBUF_OK(ip);
        return 0;
}

static int
iobuf_full(const struct iobuf *ip)
{
        IOBUF_OK(ip);
        return ip->i_outp == ip->i_out + IOBUF_SIZE;
}

int
iobuf_flush(struct iobuf *ip)
{
        const char *p = NULL;
        ssize_t n = 0;
        size_t nleft = 0;

        IOBUF_OK(ip);
        p = ip->i_out;
        nleft = (size_t)(ip->i_outp - ip->i_out);
        while (nleft > 0) {
                n = write(ip->i_fd, p, nleft);
                if (n < 0 && errno == EINTR)
                        continue;
                if (n < 0)
                        return -1;
                p += n;
                nleft -= (size_t)n;
        }

        ip->i_outp = ip->i_out;
        IOBUF_OK(ip);
        return 0;
}

ssize_t
iobuf_read(struct iobuf *ip, char *buf, size_t sz)
{
        char *p = NULL;
        size_t n = 0;
        int c = -1;

        dbug(buf == NULL, "buf == NULL");
        dbug(sz == 0, "sz == 0");
        IOBUF_OK(ip);

        p = buf;
        for (n = 0; n < sz; n++) {
                c = iobuf_getc(ip);
                if (c == IOBUF_EOF)
                        break;
                if (c < 0)
                        return -1;
                *p++ = (char)c;
        }

        return (ssize_t)(p - buf);
}
