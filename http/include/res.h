#ifndef RES_H
#define RES_H

#include "../../io/include/iobuf.h"

/* misc. constants */
enum {
        RES_HDR_VAL_SIZE = (1 << 12) - 1, /* header value size */
};

/* state types */
enum {
        RES_STATE_FIRST, /* write first line? */
        RES_STATE_HDR,   /* writing headers? */
        RES_STATE_PAY,   /* writing payload? */
        RES_STATE_COUNT, /* state count */
};

/* response header types */
enum {
        RES_HDR_CONTENT_LENGTH, /* Content-Length */
        RES_HDR_COUNT,          /* header count */
};

/* response codes */
enum {
        RES_CODE_OK,    /* OK */
        RES_CODE_COUNT, /* code count */
};

/* response version */
enum {
        RES_V_1_1,   /* HTTP/1.1 */
        RES_V_COUNT, /* version count */
};

/* invalid types */
enum {
        RES_STATE_INV = -1, /* invalid state */
        RES_CODE_INV  = -1, /* invalid code */
        RES_HDR_INV   = -1, /* invalid header */
        RES_V_INV     = -1, /* invalid version */
};

/* response */
struct res {
        struct iobuf rs_buf;                       /* buffer */
        char         rs_hdr[RES_HDR_COUNT]
                           [RES_HDR_VAL_SIZE + 1]; /* headers */
        int          rs_v;                         /* version */
        int          rs_code;                      /* code */
        int          rs_state;                     /* state */
};

/**
 * init res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_init(struct res *rsp);

/**
 * free res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_free(struct res *rsp);

/**
 * set iobuf{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *  @ip:  pointer to iobuf{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_set_buf(struct res *rsp, struct iobuf *ip);

/**
 * set version of res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *  @v:   request version type
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_set_v(struct res *rsp, int v);

/**
 * set code of res{}:
 *
 * args:
 *  @rsp:  pointer to res{}
 *  @code: code
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_set_code(struct res *rsp, int code);

/**
 * set header of res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *  @v:   value
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_set_hdr(struct res *rsp, int hdr, const char *v);

/**
 * write to res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *  @buf: buffer
 *  @sz:  length of buffer
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_write(struct res *rsp, const void *buf, size_t sz);

/**
 * write first line to res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_write_first(struct res *rsp);

/**
 * write headers to res{}:
 *
 * args:
 *  @rsp: pointer to res{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_write_hdr(struct res *rsp);

#endif /* #ifndef RES_H */
