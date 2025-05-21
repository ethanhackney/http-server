#include "../ctl/lib/include/util.h"
#include "lex.h"
#include <string.h>
#include <ctype.h>

/* hash sizes */
enum {
        HASH_METHOD_SIZE  = 1, /* size of method hash map */
        HASH_VERSION_SIZE = 1, /* size of version hash map */
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
        lp->l_class = CL_ERR;
        lp->l_type = TT_IO_ERR;

        lex_next(lp);
        return 0;
}

int
lex_free(struct lex *lp)
{
        LEX_OK(lp);

        if (iobuf_free(&lp->l_buf) < 0)
                return -1;

        memset(lp, 0, sizeof(*lp));
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
                [TT_FIRST_BAD] = "TT_FIRST_BAD",
                [TT_TOO_LONG]  = "TT_TOO_LONG",
                [TT_CRLF_ERR]  = "TT_CRLF_ERR",
                [TT_BAD_CHAR]  = "TT_BAD_CHAR",
                [TT_IO_ERR]    = "TT_IO_ERR",
                [TT_V_1_1]     = "TT_V_1_1",
                [TT_CHAR]      = "TT_CHAR",
                [TT_PATH]      = "TT_PATH",
                [TT_GET]       = "TT_GET",
                [TT_EOL]       = "TT_EOL",
                [TT_EOH]       = "TT_EOH",
                [TT_EOF]       = "TT_EOF",
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
                [CL_CHAR]    = "CL_CHAR",
                [CL_PATH]    = "CL_PATH",
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

        LEX_OK(lp);

        lp->l_last = lp->l_type;
again:
        if (lp->l_pay) {
                lex_not_special(lp);
                return;
        }

        if (lp->l_val) {
                lex_not_special(lp);
                return;
        }

        c = lex_getc(lp);
        if (c == LEX_EOF) {
                lex_set_token(lp, TT_EOF);
                return;
        }

        if (c == '\r') {
                lex_crlf(lp);
                return;
        }

        if (isspace(c))
                goto again;

        if (lp->l_first) {
                lp->l_back = c;
                lex_first(lp);
                return;
        }

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
                [TT_FIRST_BAD] = CL_ERR,
                [TT_TOO_LONG]  = CL_ERR,
                [TT_BAD_CHAR]  = CL_ERR,
                [TT_CRLF_ERR]  = CL_ERR,
                [TT_IO_ERR]    = CL_ERR,
                [TT_V_1_1]     = CL_VERSION,
                [TT_CHAR]      = CL_CHAR,
                [TT_PATH]      = CL_PATH,
                [TT_GET]       = CL_METHOD,
                [TT_EOL]       = CL_EOL,
                [TT_EOH]       = CL_EOH,
                [TT_EOF]       = CL_EOF,
        };

        lp->l_type = type;
        LEX_OK(lp);
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
                lex_set_token(lp, TT_EOL);
        }
}

static void
lex_first(struct lex *lp)
{
        static const struct kword method_hash[HASH_METHOD_SIZE] = {
                [0] = { "GET", TT_GET },
        };
        static const struct kword version_hash[HASH_VERSION_SIZE] = {
                [0] = { "HTTP/1.1", TT_V_1_1 },
        };
        const struct kword *kp = NULL;
        size_t bkt = 0;
        char *p = NULL;
        char c = -1;

        while_lex_not_full(lp, p) {
                c = lex_getc(lp);
                if (lp->l_type != TT_CHAR)
                        break;
                if (isspace(c))
                        break;
                *p = c;
        }
        lp->l_back = c;
        *p = 0;

        if (lp->l_type != TT_CHAR)
                return;

        if (!isspace(c)) {
                lex_set_token(lp, TT_TOO_LONG);
                return;
        }

        p = lp->l_lex;
        if (*p == '/') {
                lex_set_token(lp, TT_PATH);
                return;
        }

        bkt = lex_hash(p, HASH_METHOD_SIZE);
        kp = &method_hash[bkt];
        if (strcmp(p, kp->k_word) == 0) {
                lex_set_token(lp, kp->k_type);
                return;
        }

        bkt = lex_hash(p, HASH_VERSION_SIZE);
        kp = &version_hash[bkt];
        if (strcmp(p, kp->k_word) == 0) {
                lex_set_token(lp, kp->k_type);
                return;
        }

        lex_set_token(lp, TT_FIRST_BAD);
}

static size_t
lex_hash(const char *s, size_t cap)
{
        const char *p = NULL;
        size_t hash = 0;

        hash = 5381;
        for (p = s; *p != 0; p++)
                hash = hash * 31 + (size_t)*p;

        return hash % cap;
}
