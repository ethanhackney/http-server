#ifndef IOBUF_H
#define IOBUF_H

#include <stddef.h>
#include <sys/types.h>

/* misc. constants */
enum {
        IOBUF_SIZE = (1 << 13), /* buffer size */
        IOBUF_EOF  = -2,        /* end of file */
};

/* io buffer */
struct iobuf {
        char  i_in[IOBUF_SIZE];  /* private: input buffer */
        char  i_out[IOBUF_SIZE]; /* private: output buffer */
        char *i_inp;             /* private: next place to read */
        char *i_endp;            /* private: end of input data */
        char *i_outp;            /* private: next place to write */
        int   i_fd;              /* private: file descriptor */
};

/**
 * init iobuf{}:
 *
 * args:
 *  @ip: pointer to iobuf{}
 *  @fd: file descriptor
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int iobuf_init(struct iobuf *ip, int fd);

/**
 * free iobuf{}:
 *
 * args:
 *  @ip: pointer to iobuf{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int iobuf_free(struct iobuf *ip);

/**
 * get character from iobuf{}:
 *
 * args:
 *  @ip: pointer to iobuf{}:
 *
 * ret:
 *  @success: character or IOBUF_EOF
 *  @failure: -1 and errno set
 */
int iobuf_getc(struct iobuf *ip);

/**
 * move iobuf:
 *
 * args:
 *  @src: pointer to source
 *  @dst: pointer to destination
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
void iobuf_move(struct iobuf *dst, struct iobuf *src);

/**
 * printf for iobuf{}:
 *
 * args:
 *  @ip:  pointer to iobuf{}
 *  @fmt: format string
 *  @...: arguments
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int iobuf_printf(struct iobuf *ip, const char *fmt, ...);

/**
 * flush output buffer:
 *
 * args:
 *  @ip: pointer to iobuf{}:
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int iobuf_flush(struct iobuf *ip);

/**
 * read from iobuf{}:
 *
 * args:
 *  @ip:  pointer to iobuf{}:
 *  @buf: buffer
 *  @sz:  size of buf
 *
 * ret:
 *  @success: number of bytes read
 *  @failure: -1 and errno set
 */
ssize_t iobuf_read(struct iobuf *ip, char *buf, size_t sz);

#endif /* #ifndef IOBUF_H */
