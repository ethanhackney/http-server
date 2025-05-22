#ifndef REQ_H
#define REQ_H

#include "iobuf.h"
#include "lex.h"
#include <sys/socket.h>

/* ipv[46] address */
typedef struct sockaddr_storage ip_addr_t;

/* misc. constants */
enum {
        REQ_HDR_VAL_SIZE = LEX_LEX_SIZE, /* size of header value */
        REQ_URL_SIZE    = (1 << 12) - 1, /* url size */
        REQ_PAY_SIZE     = (1 << 13),    /* payload size */
};

/* methods */
enum {
        REQ_METHOD_GET,   /* (must be first) GET */
        REQ_METHOD_POST,  /* POST */
        REQ_METHOD_COUNT, /* method count */
};

/* versions */
enum {
        REQ_V_1_1,   /* (must be first) HTTP/1.1 */
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
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int req_init(struct req *rp);

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

#endif /* #ifndef REQ_H */
