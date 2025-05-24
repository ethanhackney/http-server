#ifndef RES_H
#define RES_H

#include "../../io/include/iobuf.h"

/* misc. constants */
enum {
        RES_HDR_VAL_SIZE = (1 << 10) - 1, /* header value size */
};

/* response header types */
enum {
        RES_HDR_CONTENT_LENGTH, /* Content-Length (must be first) */
        RES_HDR_COUNT,          /* header count */
};

/* response codes */
enum {
        RES_CODE_OK,    /* OK (must be first) */
        RES_CODE_COUNT, /* code count */
};

/* response version */
enum {
        RES_V_1_1,   /* HTTP/1.1 (must be first) */
        RES_V_COUNT, /* version count */
};

/* response */
struct res {
        struct iobuf rs_buf;                       /* buffer */
        char         rs_hdr[RES_HDR_COUNT]
                           [RES_HDR_VAL_SIZE + 1]; /* headers */
        int          rs_v;                         /* version */
        int          rs_code;                      /* code */
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
 *  @v:   version
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int res_set_v(struct res *rsp, int v);

#endif /* #ifndef RES_H */
