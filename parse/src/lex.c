#include "../../lib/include/util.h"
#include "../../http/include/req.h"
#include "../include/lex.h"
#include <string.h>
#include <ctype.h>

/* hash sizes */
enum {
        HASH_METHOD_SIZE  = 5, /* size of method hash map */
        HASH_VERSION_SIZE = 1, /* size of version hash map */
        HASH_HEADER_SIZE  = 5, /* size of header hash map */
};

/**
 * iterate through lp->l_lex while it has space:
 *
 * args:
 *  @_lp: pointer to lex{}
 *  @_p:  pointer
 */
#define while_lex_not_full(_lp, _p) \
        for (_p = (_lp)->l_lex; _p < (_lp)->l_lex + LEX_LEX_SIZE; _p++)

/* if debugging */
#ifdef DBUG
/**
 * validate lex{} state:
 *
 * args:
 *  @_lp: pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: exit process
 */
#define LEX_OK(_lp) do {                                                \
        size_t _len = 0;                                                \
        bool _above = false;                                            \
        bool _below = false;                                            \
        bool _in_range = false;                                         \
                                                                        \
        dbug((_lp) == NULL, "lp == NULL");                              \
                                                                        \
        _len = strnlen((_lp)->l_lex, LEX_LEX_SIZE);                     \
        dbug((_lp)->l_lex[_len] != 0, "lp->l_lex not null terminated"); \
                                                                        \
        _above = TT_INV < (_lp)->l_type;                                \
        _below = (_lp)->l_type < TT_COUNT;                              \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "lp->l_type is invalid");                      \
                                                                        \
        _above = LEX_MODE_INV < (_lp)->l_mode;                          \
        _below = (_lp)->l_mode < LEX_MODE_COUNT;                        \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "lp->l_mode is invalid");                      \
                                                                        \
        _above = CL_INV < (_lp)->l_class;                               \
        _below = (_lp)->l_class < CL_COUNT;                             \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "lp->l_class is invalid");                     \
                                                                        \
        dbug((_lp)->l_back < 0, "lp->l_back < 0");                      \
} while (0)
#else
#define LEX_OK(_lp) /* no-op */
#endif /* #ifdef DBUG */

/* keyword */
struct kword {
        const char *const k_word; /* word */
        const int         k_type; /* token type */
};
#include "../include/hash.h"

/**
 * get next char:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: character or LEX_EOF
 *  @failure: -1 and errno set
 */
static char lex_getc(struct lex *lp);

/**
 * set token type and class:
 *
 * args:
 *  @lp:   pointer to lex{}
 *  @type: token type
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
static void lex_set_token(struct lex *lp, int type);

/**
 * end of line or headers:
 *
 * args:
 *  @lp:   pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
static void lex_crlf(struct lex *lp);

/**
 * first line of request:
 *
 * args:
 *  @lp:   pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
static void lex_first(struct lex *lp);

/**
 * hash lookup:
 *
 * args:
 *  @hash: hash table
 *  @cap:  capacity of table
 *  @key:  key to lookup
 *
 * ret:
 *  @success: pointer to kword
 *  @failure: NULL
 */
static const struct kword *lex_hash_get(const struct kword *hash,
                                        size_t cap,
                                        const char *key);

/**
 * header:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
static void lex_hdr(struct lex *lp);

/**
 * value:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
static void lex_val(struct lex *lp);

int
lex_init(struct lex *lp, int fd)
{
        dbug(lp == NULL, "lp == NULL");
        dbug(fd < 0, "fd < 0");

        memset(lp, 0, sizeof(*lp));

        if (iobuf_init(&lp->l_buf, fd) < 0)
                return -1;

        lp->l_mode = LEX_MODE_FIRST;
        lp->l_class = CL_EOF;
        lp->l_type = TT_EOF;

        lex_next(lp);
        return 0;
}

int
lex_free(struct lex *lp)
{
        LEX_OK(lp);

        if (lp->l_mode != LEX_MODE_DONE) {
                if (iobuf_free(&lp->l_buf) < 0)
                        return -1;
        }

        memset(lp, 0, sizeof(*lp));
        lp->l_class = CL_INV;
        return 0;
}

int
lex_type(struct lex *lp)
{
        LEX_OK(lp);
        return lp->l_type;
}

int
lex_class(struct lex *lp)
{
        LEX_OK(lp);
        return lp->l_class;
}

const char *
lex_type_name(struct lex *lp)
{
        static const char *const names[TT_COUNT] = {
                [TT_ACCEPT_DATETIME] = "TT_ACCEPT_DATETIME",
                [TT_ACCEPT_ENCODING] = "TT_ACCEPT_ENCODING",
                [TT_ACCEPT_LANGUAGE] = "TT_ACCEPT_LANGUAGE",
                [TT_ACCEPT_CHARSET]  = "TT_ACCEPT_CHARSET",
                [TT_USER_AGENT]      = "TT_USER_AGENT",
                [TT_FIRST_BAD]       = "TT_FIRST_BAD",
                [TT_TOO_LONG]        = "TT_TOO_LONG",
                [TT_CRLF_ERR]        = "TT_CRLF_ERR",
                [TT_BAD_CHAR]        = "TT_BAD_CHAR",
                [TT_BAD_HDR]         = "TT_BAD_HDR",
                [TT_CONNECT]         = "TT_CONNECT",
                [TT_OPTIONS]         = "TT_OPTIONS",
                [TT_ACCEPT]          = "TT_ACCEPT",
                [TT_DELETE]          = "TT_DELETE",
                [TT_IO_ERR]          = "TT_IO_ERR",
                [TT_V_1_1]           = "TT_V_1_1",
                [TT_PATCH]           = "TT_PATCH",
                [TT_TRACE]           = "TT_TRACE",
                [TT_A_IM]            = "TT_A_IM",
                [TT_HEAD]            = "TT_HEAD",
                [TT_CHAR]            = "TT_CHAR",
                [TT_HOST]            = "TT_HOST",
                [TT_POST]            = "TT_POST",
                [TT_PUT]             = "TT_PUT",
                [TT_URL]             = "TT_URL",
                [TT_GET]             = "TT_GET",
                [TT_VAL]             = "TT_VAL",
                [TT_EOL]             = "TT_EOL",
                [TT_EOH]             = "TT_EOH",
                [TT_EOF]             = "TT_EOF",
        };

        LEX_OK(lp);
        return names[lp->l_type];
}

const char *
lex_class_name(struct lex *lp)
{
        static const char *const names[CL_COUNT] = {
                [CL_METHOD]  = "CL_METHOD",
                [CL_VERSION] = "CL_VERSION",
                [CL_HEADER]  = "CL_HEADER",
                [CL_CHAR]    = "CL_CHAR",
                [CL_URL]     = "CL_URL",
                [CL_EOL]     = "CL_EOL",
                [CL_EOH]     = "CL_EOH",
                [CL_ERR]     = "CL_ERR",
                [CL_EOF]     = "CL_EOF",
        };

        LEX_OK(lp);
        return names[lp->l_class];
}

void
lex_next(struct lex *lp)
{
        char c = -1;
        int last = -1;

        LEX_OK(lp);
        dbug(lp->l_mode == LEX_MODE_DONE, "lp->l_mode == LEX_MODE_DONE");

        last = lp->l_type;
again:
        c = lex_getc(lp);
        if (c == LEX_EOF) {
                lex_set_token(lp, TT_EOF);
                return;
        }

        if (c == '\r') {
                lp->l_type = last;
                lex_crlf(lp);
                return;
        }

        if (isspace(c))
                goto again;

        lp->l_back = c;

        if (lp->l_mode == LEX_MODE_VAL) {
                lex_val(lp);
                return;
        }

        if (lp->l_mode == LEX_MODE_FIRST) {
                lex_first(lp);
                return;
        }

        if (lp->l_mode == LEX_MODE_HDR) {
                lex_hdr(lp);
                return;
        }

        lp->l_back = 0;
        lp->l_lex[0] = c;
        lp->l_lex[1] = 0;
        lex_set_token(lp, TT_BAD_CHAR);
}

static char
lex_getc(struct lex *lp)
{
        int c = -1;

        LEX_OK(lp);

        c = lp->l_back;
        if (c != 0) {
                lp->l_back = 0;
                lex_set_token(lp, TT_CHAR);
                return (char)c;
        }

        c = iobuf_getc(&lp->l_buf);
        if (c == IOBUF_EOF)
                lex_set_token(lp, TT_EOF);
        else if (c < 0)
                lex_set_token(lp, TT_IO_ERR);
        else
                lex_set_token(lp, TT_CHAR);

        return (char)c;
}

static void
lex_set_token(struct lex *lp, int type)
{
#ifdef DBUG
        bool above = false;
        bool below = false;
        bool in_range = false;

        above = TT_INV < type;
        below = type < TT_COUNT;
        in_range = above && below;
        dbug(!in_range, "type is invalid");
#endif
        static const int tt_to_cl[TT_COUNT] = {
                [TT_ACCEPT_DATETIME] = CL_HEADER,
                [TT_ACCEPT_ENCODING] = CL_HEADER,
                [TT_ACCEPT_LANGUAGE] = CL_HEADER,
                [TT_ACCEPT_CHARSET]  = CL_HEADER,
                [TT_USER_AGENT]      = CL_HEADER,
                [TT_FIRST_BAD]       = CL_ERR,
                [TT_TOO_LONG]        = CL_ERR,
                [TT_BAD_CHAR]        = CL_ERR,
                [TT_CRLF_ERR]        = CL_ERR,
                [TT_CONNECT]         = CL_METHOD,
                [TT_BAD_HDR]         = CL_ERR,
                [TT_OPTIONS]         = CL_METHOD,
                [TT_ACCEPT]          = CL_HEADER,
                [TT_IO_ERR]          = CL_ERR,
                [TT_DELETE]          = CL_METHOD,
                [TT_V_1_1]           = CL_VERSION,
                [TT_TRACE]           = CL_METHOD,
                [TT_PATCH]           = CL_METHOD,
                [TT_A_IM]            = CL_HEADER,
                [TT_HOST]            = CL_HEADER,
                [TT_POST]            = CL_METHOD,
                [TT_CHAR]            = CL_CHAR,
                [TT_HEAD]            = CL_METHOD,
                [TT_PUT]             = CL_METHOD,
                [TT_URL]             = CL_URL,
                [TT_GET]             = CL_METHOD,
                [TT_EOL]             = CL_EOL,
                [TT_VAL]             = CL_VAL,
                [TT_EOH]             = CL_EOH,
                [TT_EOF]             = CL_EOF,
        };

        LEX_OK(lp);
        lp->l_last = lp->l_type;
        lp->l_type = type;
        lp->l_class = tt_to_cl[lp->l_type];
}

const char *
lex_lex(struct lex *lp)
{
        LEX_OK(lp);
        return lp->l_lex;
}

static void
lex_crlf(struct lex *lp)
{
        char c = -1;

        LEX_OK(lp);

        c = lex_getc(lp);
        lp->l_lex[0] = '\r';
        lp->l_lex[1] = c;
        lp->l_lex[2] = 0;

        if (c != '\n') {
                lex_set_token(lp, TT_CRLF_ERR);
        } else if (lp->l_last == TT_EOL) {
                lp->l_mode = LEX_MODE_DONE;
                lex_set_token(lp, TT_EOH);
        } else {
                lp->l_mode = LEX_MODE_HDR;
                lex_set_token(lp, TT_EOL);
        }
}

static void
lex_first(struct lex *lp)
{
        const struct kword *kp = NULL;
        char *p = NULL;
        char c = -1;

        LEX_OK(lp);

        while_lex_not_full(lp, p) {
                c = lex_getc(lp);
                if (isspace(c))
                        break;
                *p = c;
                if (lp->l_type != TT_CHAR)
                        break;
        }
        *p = 0;

        if (lp->l_type != TT_CHAR)
                return;

        lp->l_back = c;

        if (!isspace(c)) {
                lex_set_token(lp, TT_TOO_LONG);
                return;
        }

        p = lp->l_lex;
        if (*p == '/') {
                lex_set_token(lp, TT_URL);
                return;
        }

        kp = lex_hash_get(method_hash, method_hash_cap, p);
        if (kp != NULL) {
                lex_set_token(lp, kp->k_type);
                return;
        }

        kp = lex_hash_get(version_hash, version_hash_cap, p);
        if (kp != NULL) {
                lex_set_token(lp, kp->k_type);
                return;
        }

        lex_set_token(lp, TT_FIRST_BAD);
}

static const struct kword *
lex_hash_get(const struct kword *hash, size_t cap, const char *key)
{
        const struct kword *kp = NULL;
        size_t bkt = 0;

        dbug(hash == NULL, "hash == NULL");
        dbug(cap == 0, "cap == 0");
        dbug(key == NULL, "key == NULL");

        bkt = str_hash(key, cap);
        kp = &hash[bkt];

        if (kp->k_word == NULL)
                return NULL;

        if (strcmp(key, kp->k_word) != 0)
                return NULL;

        return kp;
}

static void
lex_hdr(struct lex *lp)
{
        const struct kword *kp = NULL;
        char *p = NULL;
        char c = -1;

        LEX_OK(lp);

        while_lex_not_full(lp, p) {
                c = lex_getc(lp);
                if (c == ':')
                        break;
                *p = c;
                if (lp->l_type != TT_CHAR)
                        break;
        }
        *p = 0;

        if (lp->l_type != TT_CHAR)
                return;

        if (c != ':') {
                lp->l_back = c;
                lex_set_token(lp, TT_TOO_LONG);
                return;
        }

        kp = lex_hash_get(hdr_hash, hdr_hash_cap, lp->l_lex);
        if (kp != NULL) {
                lp->l_mode = LEX_MODE_VAL;
                lex_set_token(lp, kp->k_type);
                return;
        }

        lex_set_token(lp, TT_BAD_HDR);
}

static void
lex_val(struct lex *lp)
{
        char *p = NULL;
        char c = -1;

        LEX_OK(lp);

        while_lex_not_full(lp, p) {
                c = lex_getc(lp);
                if (c == '\r')
                        break;
                *p = c;
                if (lp->l_type != TT_CHAR)
                        break;
        }
        lp->l_back = c;
        *p = 0;

        if (lp->l_type != TT_CHAR)
                return;

        if (c != '\r') {
                lex_set_token(lp, TT_TOO_LONG);
                return;
        }

        lp->l_mode = LEX_MODE_HDR;
        lex_set_token(lp, TT_VAL);
}

bool
lex_empty(const struct lex *lp)
{
        LEX_OK(lp);
        return lp->l_lex[0] == 0;
}

int
lex_buf_move(struct lex *lp, struct req *rp)
{
        LEX_OK(lp);
        dbug(lp->l_mode != LEX_MODE_DONE, "lp->l_mode != LEX_MODE_DONE");
        dbug(rp == NULL, "rp == NULL");

        return req_set_buf(rp, &lp->l_buf);
}
