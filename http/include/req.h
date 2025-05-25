#ifndef REQ_H
#define REQ_H

#include "../../io/include/iobuf.h"
#include "res.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stddef.h>

/* ipv[46] address */
typedef struct sockaddr_storage ip_addr_t;

/* misc. constants */
enum {
        REQ_HDR_VAL_SIZE = (1 << 10) - 1, /* size of header value */
        REQ_URL_SIZE     = (1 << 12) - 1, /* url size */
        REQ_PAY_SIZE     = (1 << 13),     /* payload size */
};

/* methods */
enum {
        REQ_METHOD_GET,   /* GET (must be first) */
        REQ_METHOD_POST,  /* POST */
        REQ_METHOD_COUNT, /* method count */
};

/* versions */
enum {
        REQ_V_1_1,   /* HTTP/1.1 (must be first) */
        REQ_V_COUNT, /* version count */
};

/* header types */
enum {
        REQ_HDR_HOST,       /* Host (must be first) */
        REQ_HDR_USER_AGENT, /* User-Agent */
        REQ_HDR_ACCEPT,     /* Accept */
        REQ_HDR_COUNT,      /* header count */
};

/* request */
struct req {
        struct iobuf r_buf;                       /* private: buffer */
        ip_addr_t    r_addr;                      /* public: client address */
        char         r_hdr[REQ_HDR_COUNT]
                          [REQ_HDR_VAL_SIZE + 1]; /* public: headers */
        char         r_pay[REQ_PAY_SIZE];         /* public: payload */
        char         r_url[REQ_URL_SIZE + 1];     /* public: url */
        int          r_method;                    /* public: method */
        int          r_v;                         /* public: version */
};

/**
 * init req{}:
 *
 * args:
 *  @rp: pointer to req{}
 *  @sp: pointer to client address
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_init(struct req *rp, const struct sockaddr_storage *sp);

/**
 * free req{}:
 *
 * args:
 *  @rp: pointer to req{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_free(struct req *rp);

/**
 * set iobuf{}:
 *
 * args:
 *  @rp: pointer to req{}
 *  @ip: pointer to iobuf{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_set_buf(struct req *rp, struct iobuf *ip);

/**
 * set method:
 *
 * args:
 *  @rp:   pointer to req{}
 *  @type: token type
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_set_method(struct req *rp, int type);

/**
 * get method name:
 *
 * args:
 *  @rp:   pointer to req{}
 *
 * ret:
 *  @success: pointer to method name
 *  @failure: NULL
 */
const char *req_method_name(struct req *rp);

/**
 * set version:
 *
 * args:
 *  @rp:   pointer to req{}
 *  @type: token type
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_set_v(struct req *rp, int type);

/**
 * get version name:
 *
 * args:
 *  @rp:   pointer to req{}
 *
 * ret:
 *  @success: pointer to version name
 *  @failure: NULL
 */
const char *req_v_name(struct req *rp);

/**
 * read from req{}:
 *
 * args:
 *  @rp:  pointer to req{}
 *  @buf: buffer
 *  @sz:  size of buffer
 *
 * ret:
 *  @success: number of bytes read
 *  @failure: -1 and errno set
 */
ssize_t req_read(struct req *rp, char *buf, size_t sz);

/**
 * move r_buf to rs_buf:
 *
 * args:
 *  @rp:  pointer to req{}
 *  @rsp: pointer to res{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_buf_move(struct req *rp, struct res *rsp);

/**
 * set header:
 *
 * args:
 *  @rp:  pointer to req{}
 *  @hdr: header
 *  @val: value
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_set_hdr(struct req *rp, int hdr, const char *val);

/**
 * get name of header:
 *
 * args:
 *  @type: header type
 *
 * ret:
 *  @success: string
 *  @failure: does not
 */
const char *req_hdr_name(int type);

#endif /* #ifndef REQ_H */
