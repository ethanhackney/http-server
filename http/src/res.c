#include "../../lib/include/util.h"
#include "../include/res.h"
#include "../include/req.h"
#include <string.h>

/* if debugging */
#ifdef DBUG
/**
 * validate res{} state:
 *
 * args:
 *  @_rsp: pointer to res{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define RES_OK(_rsp) do {                                               \
        const char *_v = NULL;                                          \
        size_t _i = 0;                                                  \
        size_t _len = 0;                                                \
        bool _above = false;                                            \
        bool _below = false;                                            \
        bool _in_range = false;                                         \
                                                                        \
        dbug((_rsp) == NULL, "rsp == NULL");                            \
                                                                        \
        _above = RES_STATE_INV < (_rsp)->rs_state;                      \
        _below = (_rsp)->rs_state < RES_STATE_COUNT;                    \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rsp->rs_state state invalid");                \
                                                                        \
        for (_i = RES_HDR_INV + 1; _i < RES_HDR_COUNT; _i++) {          \
                _v = (_rsp)->rs_hdr[_i];                                \
                _len = strnlen(_v, RES_HDR_VAL_SIZE);                   \
                dbug(_v[_len] != 0, "rsp->rs_hdr end not null");        \
        }                                                               \
} while (0)

/**
 * validate first line:
 *
 * args:
 *  @_rsp: pointer to res{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define RES_FIRST_OK(_rsp) do {                                         \
        const char *_v = NULL;                                          \
        size_t _i = 0;                                                  \
        size_t _len = 0;                                                \
        bool _above = false;                                            \
        bool _below = false;                                            \
        bool _in_range = false;                                         \
                                                                        \
        dbug((_rsp) == NULL, "rsp == NULL");                            \
                                                                        \
        _above = RES_V_INV < (_rsp)->rs_v;                              \
        _below = (_rsp)->rs_v < RES_V_COUNT;                            \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rsp->rs_v version invalid");                  \
                                                                        \
        _above = RES_CODE_INV < (_rsp)->rs_code;                        \
        _below = (_rsp)->rs_code < RES_CODE_COUNT;                      \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rsp->rs_code code invalid");                  \
                                                                        \
        for (_i = RES_HDR_INV + 1; _i < RES_HDR_COUNT; _i++) {          \
                _v = (_rsp)->rs_hdr[_i];                                \
                _len = strnlen(_v, RES_HDR_VAL_SIZE);                   \
                dbug(_v[_len] != 0, "rsp->rs_hdr end not null");        \
        }                                                               \
} while (0)
#else
#define RES_OK(_rsp)       /* no-op */
#define RES_FIRST_OK(_rsp) /* no-op */
#endif /* #ifdef DBUG */

int
res_init(struct res *rsp)
{
        memset(rsp, 0, sizeof(*rsp));
        rsp->rs_state = RES_STATE_FIRST;
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
        dbug(rsp->rs_state != RES_STATE_PAY, "rsp->rs_state != RES_STATE_PAY");

        return iobuf_write(&rsp->rs_buf, buf, sz);
}

int
res_write_first(struct res *rsp)
{
        static const char *const v[RES_V_COUNT] = {
                [RES_V_1_1] = "HTTP/1.1",
        };
        static const char *const res_code[RES_CODE_COUNT] = {
                [RES_CODE_OK] = "200",
        };
        static const char *const res_msg[RES_CODE_COUNT] = {
                [RES_CODE_OK] = "OK",
        };
        char buf[1024] = "";
        int ret = -1;

        RES_OK(rsp);
        RES_FIRST_OK(rsp);
        dbug(rsp->rs_state != RES_STATE_FIRST,
             "rsp->rs_state != RES_STATE_FIRST");

        ret = snprintf(buf,
                      sizeof(buf),
                      "%s %s %s\r\n",
                      v[rsp->rs_v],
                      res_code[rsp->rs_code],
                      res_msg[rsp->rs_code]);
        if (ret < 0)
                return -1;

        if (iobuf_write(&rsp->rs_buf, buf, strlen(buf)) < 0)
                return -1;

        rsp->rs_state = RES_STATE_HDR;
        return 0;
}

int
res_write_hdr(struct res *rsp)
{
        static const char *const hdr[RES_HDR_COUNT] = {
                [RES_HDR_CONTENT_LENGTH] = "Content-Length",
        };
        const char *v = NULL;
        const char *h = NULL;
        char buf[1024] = "";
        size_t i = 0;
        int ret = -1;

        RES_OK(rsp);
        dbug(rsp->rs_state != RES_STATE_HDR, "rsp->rs_state != RES_STATE_HDR");

        for (i = RES_HDR_INV + 1; i < RES_HDR_COUNT; i++) {
                v = rsp->rs_hdr[i];
                if (*v == 0)
                        continue;

                h = hdr[i];
                ret = snprintf(buf, sizeof(buf), "%s: ", h);
                if (ret < 0)
                        return -1;

                if (iobuf_write(&rsp->rs_buf, buf, strlen(buf)) < 0)
                        return -1;

                if (iobuf_write(&rsp->rs_buf, v, strlen(v)) < 0)
                        return -1;

                if (iobuf_write(&rsp->rs_buf, "\r\n", 2) < 0)
                        return -1;
        }

        if (iobuf_write(&rsp->rs_buf, "\r\n", 2) < 0)
                return -1;

        rsp->rs_state = RES_STATE_PAY;
        return 0;
}

int
res_set_code(struct res *rsp, int code)
{
        RES_OK(rsp);
        dbug(code <= RES_CODE_INV || code >= RES_CODE_COUNT, "code is invalid");
        dbug(rsp->rs_state != RES_STATE_FIRST,
             "rsp->rs_state != RES_STATE_FIRST");

        rsp->rs_code = code;
        return 0;
}

int
res_set_v(struct res *rsp, int v)
{
        static const int req_2_res[REQ_V_COUNT] = {
                [REQ_V_1_1] = RES_V_1_1,
        };
        RES_OK(rsp);
        dbug(v <= REQ_V_INV || v >= REQ_V_COUNT, "v is invalid");
        dbug(rsp->rs_state != RES_STATE_FIRST,
             "rsp->rs_state != RES_STATE_FIRST");

        rsp->rs_v = req_2_res[v];
        return 0;
}

int
res_set_hdr(struct res *rsp, int hdr, const char *v)
{
        RES_OK(rsp);
        dbug(rsp->rs_state != RES_STATE_HDR,
             "rsp->rs_state != RES_STATE_HDR");
        dbug(hdr <= RES_HDR_INV || hdr >= RES_HDR_COUNT,
             "hdr invalid");
        dbug(v == NULL, "v == NULL");

        strcpy(rsp->rs_hdr[hdr], v);
        return 0;
}
