#include "../ctl/lib/include/util.h"
#include "iobuf.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
