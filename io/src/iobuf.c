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
#define IOBUF_OK(_ip) /* no-op */
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

        ip->i_inp = ip->i_in;
        ip->i_endp = ip->i_in + n;
        if (n == 0)
                return IOBUF_EOF;

        IOBUF_OK(ip);
        return 0;
}

void
iobuf_move(struct iobuf *dst, struct iobuf *src)
{
        dbug(dst == NULL, "dst == NULL");
        IOBUF_OK(src);

        dst->i_fd = src->i_fd;

        memcpy(dst->i_in, src->i_in, sizeof(dst->i_in));
        dst->i_inp = dst->i_in + (src->i_inp - src->i_in);
        dst->i_endp = dst->i_in + (src->i_endp - src->i_in);

        memcpy(dst->i_out, src->i_out, sizeof(dst->i_out));
        dst->i_outp = (dst->i_out + (src->i_outp - src->i_out));

        src->i_fd = -1;
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
iobuf_read(struct iobuf *ip, void *buf, size_t sz)
{
        char *p = NULL;
        size_t ncopy = 0;
        size_t nleft = 0;
        int ret = -1;

        dbug(buf == NULL, "buf == NULL");
        dbug(sz == 0, "sz == 0"); IOBUF_OK(ip);

        p = buf;
        nleft = sz;
        while (nleft > 0) {
                if (iobuf_empty(ip) < 0) {
                        ret = iobuf_fill(ip);
                        if (ret == IOBUF_EOF)
                                break;
                        if (ret < 0)
                                return ret;
                }
                ncopy = min(nleft, (size_t)(ip->i_endp - ip->i_inp));
                memcpy(p, ip->i_inp, ncopy);
                p += ncopy;
                ip->i_inp += ncopy;
                nleft -= ncopy;
        }

        return (ssize_t)(p - (char *)buf);
}

int
iobuf_write(struct iobuf *ip, const void *buf, size_t sz)
{
        const char *end = NULL;
        const char *p = NULL;
        size_t nleft = 0;
        size_t ncopy = 0;

        IOBUF_OK(ip);
        dbug(buf == NULL, "buf == NULL");
        dbug(sz == 0, "sz == 0");

        p = buf;
        end = ip->i_out + IOBUF_SIZE;
        nleft = sz;
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
