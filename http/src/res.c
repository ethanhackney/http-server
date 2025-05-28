#include "../../lib/include/util.h"
#include "../include/res.h"
#include <string.h>

/* if debugging */
#ifdef DBUG
/**
 * validate res{} state:
 *
 * args:
 *  @_rp: pointer to res{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define RES_OK(_rp) do {                                                \
        const char *_v = NULL;                                          \
        size_t _i = 0;                                                  \
        size_t _len = 0;                                                \
        bool _above = false;                                            \
        bool _below = false;                                            \
        bool _in_range = false;                                         \
                                                                        \
        dbug((_rp) == NULL, "rsp == NULL");                             \
                                                                        \
        _above = RES_CODE_OK <= (_rp)->rs_code;                         \
        _below = (_rp)->rs_code < RES_CODE_COUNT;                       \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rsp->rs_code invalid");                       \
                                                                        \
        _above = RES_V_1_1 <= (_rp)->rs_v;                              \
        _below = (_rp)->rs_v < RES_V_COUNT;                             \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rsp->rs_v invalid");                          \
                                                                        \
        for (_i = RES_CODE_OK; _i < RES_CODE_COUNT; _i++) {             \
                _v = (_rp)->rs_hdr[_i];                                 \
                _len = strnlen(_v, RES_HDR_VAL_SIZE);                   \
                dbug(_v[_len] != 0, "rp->rs_hdr end not null");         \
        }                                                               \
} while (0)
#else
#define RES_OK(_rp) /* no-op */
#endif /* #ifdef DBUG */

int
res_init(struct res *rsp)
{
        memset(rsp, 0, sizeof(*rsp));
        return 0;
}

int
res_free(struct res *rsp)
{
        RES_OK(rsp);

        if (iobuf_free(&rsp->rs_buf) < 0)
                return -1;

        memset(rsp, 0, sizeof(*rsp));
        return 0;
}

int
res_set_buf(struct res *rsp, struct iobuf *ip)
{
        dbug(ip == NULL, "ip == NULL");
        RES_OK(rsp);
        iobuf_move(&rsp->rs_buf, ip);
        return 0;
}

int
res_write(struct res *rsp, const void *buf, size_t sz)
{
        RES_OK(rsp);
        dbug(buf == NULL, "buf == NULL");
        dbug(sz == 0, "sz == 0");

        return iobuf_write(&rsp->rs_buf, buf, sz);
}
