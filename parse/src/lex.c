#include "../../lib/include/util.h"
#include "../include/lex.h"
#include "../include/req.h"
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
        _above = TT_IO_ERR <= (_lp)->l_type;                            \
        _below = (_lp)->l_type < TT_COUNT;                              \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "lp->l_type is invalid");                      \
                                                                        \
        _above = CL_ERR <= (_lp)->l_class;                              \
        _below = (_lp)->l_class < CL_COUNT;                             \
        _in_range = _above && _below;                                   \
        dbug(!_in_range, "lp->l_class is invalid");                     \
                                                                        \
        if ((_lp)->l_pay)                                               \
                break;                                                  \
                                                                        \
        dbug((_lp)->l_back < 0, "lp->l_back < 0");                      \
} while (0)
#else
#define LEX_OK(_lp) do {        \
        /* no-op */             \
} while (0)
#endif /* #ifdef DBUG */

/* keyword */
struct kword {
        const char *const k_word; /* word */
        const int         k_type; /* token type */
};

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
 * when characters are not special:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
static void lex_not_special(struct lex *lp);

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
 * string hash function:
 *
 * args:
 *  @s:   string to hash
 *  @cap: capacity of hash map
 *
 * ret:
 *  @success: hash of s
 *  @failure: does not
 */
static size_t lex_hash(const char *s, size_t cap);

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

        /* not needed, but i feel it is in bad taste to assume
         * that 'false' is actually 0, bool is an abstraction and
         * i would like to keep it that way
         */
        lp->l_hdr = false;
        lp->l_val = false;
        lp->l_pay = false;

        lp->l_first = true;
        lp->l_class = CL_EOF;
        lp->l_type = TT_EOF;

        lex_next(lp);
        return 0;
}

int
lex_free(struct lex *lp)
{
        LEX_OK(lp);
        memset(lp, 0, sizeof(*lp));
        lp->l_class = CL_EOF - 1;
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
                [TT_USER_AGENT] = "TT_USER_AGENT",
                [TT_FIRST_BAD]  = "TT_FIRST_BAD",
                [TT_TOO_LONG]   = "TT_TOO_LONG",
                [TT_CRLF_ERR]   = "TT_CRLF_ERR",
                [TT_BAD_CHAR]   = "TT_BAD_CHAR",
                [TT_BAD_HDR]    = "TT_BAD_HDR",
                [TT_ACCEPT]     = "TT_ACCEPT",
                [TT_IO_ERR]     = "TT_IO_ERR",
                [TT_V_1_1]      = "TT_V_1_1",
                [TT_CHAR]       = "TT_CHAR",
                [TT_HOST]       = "TT_HOST",
                [TT_POST]       = "TT_POST",
                [TT_URL]        = "TT_URL",
                [TT_GET]        = "TT_GET",
                [TT_VAL]        = "TT_VAL",
                [TT_EOL]        = "TT_EOL",
                [TT_EOH]        = "TT_EOH",
                [TT_EOF]        = "TT_EOF",
        };

        LEX_OK(lp);
        return names[lp->l_type];
}

const char *
lex_class_name(struct lex *lp)
{
        static const char *const names[TT_COUNT] = {
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

        if (lp->l_pay) {
                lex_not_special(lp);
                return;
        }
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

        if (lp->l_val) {
                lex_val(lp);
                return;
        }

        if (lp->l_first) {
                lex_first(lp);
                return;
        }

        if (lp->l_hdr) {
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
lex_not_special(struct lex *lp)
{
        char *p = NULL;

        LEX_OK(lp);

        while_lex_not_full(lp, p) {
                *p = lex_getc(lp);
                if (lp->l_type != TT_CHAR)
                        break;
        }
        *p = 0;
}

static void
lex_set_token(struct lex *lp, int type)
{
#ifdef DBUG
        bool above = false;
        bool below = false;
        bool in_range = false;

        above = TT_IO_ERR <= type;
        below = type < TT_COUNT;
        in_range = above && below;
        dbug(!in_range, "type is invalid");
#endif
        static const int tt_to_cl[TT_COUNT] = {
                [TT_USER_AGENT] = CL_HEADER,
                [TT_FIRST_BAD]  = CL_ERR,
                [TT_TOO_LONG]   = CL_ERR,
                [TT_BAD_CHAR]   = CL_ERR,
                [TT_CRLF_ERR]   = CL_ERR,
                [TT_BAD_HDR]    = CL_ERR,
                [TT_ACCEPT]     = CL_HEADER,
                [TT_IO_ERR]     = CL_ERR,
                [TT_V_1_1]      = CL_VERSION,
                [TT_HOST]       = CL_HEADER,
                [TT_POST]       = CL_METHOD,
                [TT_CHAR]       = CL_CHAR,
                [TT_URL]        = CL_URL,
                [TT_GET]        = CL_METHOD,
                [TT_EOL]        = CL_EOL,
                [TT_VAL]        = CL_VAL,
                [TT_EOH]        = CL_EOH,
                [TT_EOF]        = CL_EOF,
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
                lp->l_hdr = false;
                lp->l_pay = true;
                lex_set_token(lp, TT_EOH);
        } else {
                lp->l_first = false;
                lp->l_hdr = true;
                lex_set_token(lp, TT_EOL);
        }
}

static void
lex_first(struct lex *lp)
{
        static const struct kword method_hash[HASH_METHOD_SIZE] = {
                [0] = { "GET",  TT_GET },
                [2] = { "POST", TT_POST },
        };
        static const struct kword version_hash[HASH_VERSION_SIZE] = {
                [0] = { "HTTP/1.1", TT_V_1_1 },
        };
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

        kp = lex_hash_get(method_hash, HASH_METHOD_SIZE, p);
        if (kp != NULL) {
                lex_set_token(lp, kp->k_type);
                return;
        }

        kp = lex_hash_get(version_hash, HASH_VERSION_SIZE, p);
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

        bkt = lex_hash(key, cap);
        kp = &hash[bkt];

        if (kp->k_word == NULL)
                return NULL;

        if (strcmp(key, kp->k_word) != 0)
                return NULL;

        return kp;
}

static size_t
lex_hash(const char *s, size_t cap)
{
        const char *p = NULL;
        size_t hash = 0;

        dbug(s == NULL, "s == NULL");
        dbug(cap == 0, "cap == 0");

        hash = 5381;
        for (p = s; *p != 0; p++)
                hash = hash * 31 + (size_t)*p;

        return hash % cap;
}

static void
lex_hdr(struct lex *lp)
{
        static const struct kword hdr_hash[HASH_HEADER_SIZE] = {
                [1] = { "User-Agent", TT_USER_AGENT },
                [3] = { "Accept",     TT_ACCEPT     },
                [0] = { "Host",       TT_HOST       },
        };
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

        kp = lex_hash_get(hdr_hash, HASH_HEADER_SIZE, lp->l_lex);
        if (kp != NULL) {
                lp->l_hdr = false;
                lp->l_val = true;
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

        lp->l_val = false;
        lp->l_hdr = true;
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
        dbug(rp == NULL, "rp == NULL");

        return req_set_buf(rp, &lp->l_buf);
}
