#include "../../lib/include/util.h"
#include "../include/req.h"
#include <string.h>

/* if debugging */
#ifdef DBUG
/**
 * validate req{} state:
 *
 * args:
 *  @_rp: pointer to req{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define REQ_OK(_rp) do {                                                \
        const char *_v = NULL;                                          \
        size_t _i = 0;                                                  \
        size_t _len = 0;                                                \
        bool _above = false;                                            \
        bool _below = false;                                            \
        bool _in_range = false;                                         \
        int _fam = -1;                                                  \
        int _ip = -1;                                                   \
                                                                        \
        dbug((_rp) == NULL, "rp == NULL");                              \
                                                                        \
        _fam = (_rp)->r_addr.ss_family;                                 \
        _ip = _fam == AF_INET || _fam == AF_INET6;                      \
        dbug(!_ip, "rp->r_addr not ip");                                \
                                                                        \
        _len = strnlen((_rp)->r_path, REQ_PATH_SIZE);                   \
        dbug((_rp)->r_path[_len] != 0, "rp->r_path end not null");      \
                                                                        \
        _above = REQ_METHOD_GET <= (_rp)->r_method;                     \
        _below = (_rp)->r_method < REQ_METHOD_COUNT;                    \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rp->r_method invalid");                       \
                                                                        \
        _above = REQ_V_1_1 <= (_rp)->r_method;                          \
        _below = (_rp)->r_method < REQ_V_COUNT;                         \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rp->r_v invalid");                            \
                                                                        \
        for (_i = REQ_HDR_HOST; _i < REQ_HDR_COUNT; _i++) {             \
                _v = (_rp)->r_hdr[_i];                                  \
                _len = strnlen(_v, REQ_HDR_VAL_SIZE);                   \
                dbug(_v[_len] != 0, "rp->r_hdr end not null");          \
        }                                                               \
} while (0)
#else
#define REQ_OK(_rp) do {        \
        /* no-op */             \
} while (0)
#endif /* #ifdef DBUG */

int
req_init(struct req *rp, struct iobuf *ip)
{
        memset(rp, 0, sizeof(*rp));
        iobuf_move(&rp->r_buf, ip);
        return 0;
}

int
req_free(struct req *rp)
{
        REQ_OK(rp);

        if (iobuf_free(&rp->r_buf) < 0)
                return -1;

        memset(rp, 0, sizeof(*rp));
        rp->r_method = -1;
        return -1;
}
