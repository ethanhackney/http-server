#include "../../lib/include/util.h"
#include "../include/req.h"
#include "../include/lex.h"
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
        _len = strnlen((_rp)->r_url, REQ_URL_SIZE);                     \
        dbug((_rp)->r_url[_len] != 0, "rp->r_url end not null");        \
                                                                        \
        _above = REQ_METHOD_GET <= (_rp)->r_method;                     \
        _below = (_rp)->r_method < REQ_METHOD_COUNT;                    \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rp->r_method invalid");                       \
                                                                        \
        _above = REQ_V_1_1 <= (_rp)->r_v;                               \
        _below = (_rp)->r_v < REQ_V_COUNT;                              \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "rp->r_v invalid");                            \
                                                                        \
        for (_i = REQ_HDR_HOST; _i < REQ_HDR_COUNT; _i++) {             \
                _v = (_rp)->r_hdr[_i];                                  \
                _len = strnlen(_v, REQ_HDR_VAL_SIZE);                   \
                dbug(_v[_len] != 0, "rp->r_hdr end not null");          \
        }                                                               \
} while (0)
/**
 * validate req{} state during parsing:
 *
 * args:
 *  @_rp: pointer to req{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define REQ_PARSE_OK(_rp) do {                                          \
        const char *_v = NULL;                                          \
        size_t _i = 0;                                                  \
        size_t _len = 0;                                                \
        int _fam = -1;                                                  \
        int _ip = -1;                                                   \
                                                                        \
        dbug((_rp) == NULL, "rp == NULL");                              \
                                                                        \
        _fam = (_rp)->r_addr.ss_family;                                 \
        _ip = _fam == AF_INET || _fam == AF_INET6;                      \
        dbug(!_ip, "rp->r_addr not ip");                                \
                                                                        \
        _len = strnlen((_rp)->r_url, REQ_URL_SIZE);                     \
        dbug((_rp)->r_url[_len] != 0, "rp->r_url end not null");        \
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
req_init(struct req *rp, const struct sockaddr_storage *sp)
{
        dbug(rp == NULL, "rp == NULL");
        dbug(sp == NULL, "sp == NULL");
        memset(rp, 0, sizeof(*rp));
        memcpy(&rp->r_addr, sp, sizeof(*sp));
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
        return 0;
}

int
req_set_buf(struct req *rp, struct iobuf *ip)
{
        dbug(ip == NULL, "ip == NULL");
        REQ_OK(rp);
        iobuf_move(&rp->r_buf, ip);
        return 0;
}

int
req_set_method(struct req *rp, int type)
{
        static const int tt_to_method[TT_COUNT] = {
                [TT_USER_AGENT] = -1,
                [TT_FIRST_BAD]  = -1,
                [TT_TOO_LONG]   = -1,
                [TT_CRLF_ERR]   = -1,
                [TT_BAD_CHAR]   = -1,
                [TT_BAD_HDR]    = -1,
                [TT_ACCEPT]     = -1,
                [TT_IO_ERR]     = -1,
                [TT_V_1_1]      = -1,
                [TT_CHAR]       = -1,
                [TT_HOST]       = -1,
                [TT_URL]        = -1,
                [TT_POST]       = REQ_METHOD_POST,
                [TT_GET]        = REQ_METHOD_GET,
                [TT_VAL]        = -1,
                [TT_EOL]        = -1,
                [TT_EOH]        = -1,
                [TT_EOF]        = -1,
        };
        struct iobuf zero = {0};
        int m = -1;

        if (memcmp(&rp->r_buf, &zero, sizeof(zero)) == 0) {
                REQ_PARSE_OK(rp);
        } else {
                REQ_OK(rp);
        }

        m = tt_to_method[type];
        dbug(m < 0, "method type invalid");
        rp->r_method = m;
        return 0;
}

const char *
req_method_name(struct req *rp)
{
        static const char *const names[REQ_METHOD_COUNT] = {
                [REQ_METHOD_POST] = "REQ_METHOD_POST",
                [REQ_METHOD_GET]  = "REQ_METHOD_GET",
        };

        REQ_OK(rp);
        return names[rp->r_method];
}

int
req_set_v(struct req *rp, int type)
{
        static const int tt_to_v[TT_COUNT] = {
                [TT_USER_AGENT] = -1,
                [TT_FIRST_BAD]  = -1,
                [TT_TOO_LONG]   = -1,
                [TT_CRLF_ERR]   = -1,
                [TT_BAD_CHAR]   = -1,
                [TT_BAD_HDR]    = -1,
                [TT_ACCEPT]     = -1,
                [TT_IO_ERR]     = -1,
                [TT_V_1_1]      = REQ_V_1_1,
                [TT_CHAR]       = -1,
                [TT_HOST]       = -1,
                [TT_URL]        = -1,
                [TT_POST]       = -1,
                [TT_GET]        = -1,
                [TT_VAL]        = -1,
                [TT_EOL]        = -1,
                [TT_EOH]        = -1,
                [TT_EOF]        = -1,
        };
        struct iobuf zero = {0};
        int v = -1;

        if (memcmp(&rp->r_buf, &zero, sizeof(zero)) == 0) {
                REQ_PARSE_OK(rp);
        } else {
                REQ_OK(rp);
        }

        v = tt_to_v[type];
        dbug(v < 0, "version type invalid");
        rp->r_v = v;
        return 0;
}

const char *
req_v_name(struct req *rp)
{
        static const char *const names[REQ_V_COUNT] = {
                [REQ_V_1_1] = "REQ_V_1_1",
        };

        REQ_OK(rp);
        return names[rp->r_v];
}

ssize_t
req_read(struct req *rp, char *buf, size_t sz)
{
        dbug(buf == NULL, "buf == NULL");
        dbug(sz == 0, "sz == 0");
        REQ_OK(rp);

        return iobuf_read(&rp->r_buf, buf, sz);
}
