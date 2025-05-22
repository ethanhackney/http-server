#ifndef REQ_H
#define REQ_H

#include "iobuf.h"
#include "lex.h"
#include <sys/socket.h>

/* ipv[46] address */
typedef struct sockaddr_storage ip_addr_t;

/* misc. constants */
enum {
        REQ_HDR_VAL_SIZE = LEX_LEX_SIZE, /* max size of header value */
};

/* header types */
enum {
        REQ_HDR_HOST,       /* Host */
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
};

#endif /* #ifndef REQ_H */
