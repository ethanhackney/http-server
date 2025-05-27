#ifndef LEX_H
#define LEX_H

#include "../../io/include/iobuf.h"
#include <stdbool.h>

/* request */
struct req;

/* misc. constants */
enum {
        LEX_LEX_SIZE = (1 << 12) - 1, /* lexeme size */
        LEX_EOF      = IOBUF_EOF,     /* end of file */
};

/* lex mode bits */
enum {
        LEX_MODE_FIRST, /* on first line? */
        LEX_MODE_HDR,   /* in header name? */
        LEX_MODE_VAL,   /* in header value? */
        LEX_MODE_PAY,   /* in payload? */
        LEX_MODE_COUNT, /* mode count */
};

/* token types */
enum {
        TT_USER_AGENT, /* User-Agent */
        TT_FIRST_BAD,  /* token bad for first line */
        TT_CRLF_ERR,   /* \r not followed by \n */
        TT_BAD_CHAR,   /* invalid character */
        TT_TOO_LONG,   /* lexeme overflow */
        TT_BAD_HDR,    /* bad header */
        TT_CONNECT,    /* CONNECT */
        TT_OPTIONS,    /* OPTIONS */
        TT_IO_ERR,     /* io error */
        TT_ACCEPT,     /* Accept */
        TT_DELETE,     /* DELETE */
        TT_TRACE,      /* TRACE */
        TT_V_1_1,      /* HTTP/1.1 */
        TT_PATCH,      /* PATCH */
        TT_A_IM,       /* A-IM */
        TT_HEAD,       /* HEAD */
        TT_HOST,       /* Host */
        TT_POST,       /* POST */
        TT_CHAR,       /* regular character */
        TT_PUT,        /* PUT */
        TT_URL,        /* url */
        TT_EOF,        /* end of file */
        TT_EOL,        /* end of line */
        TT_EOH,        /* end of headers */
        TT_GET,        /* GET */
        TT_VAL,        /* header value */
        TT_COUNT,      /* token type count */
};

/* token classes */
enum {
        CL_VERSION, /* version */
        CL_METHOD,  /* method */
        CL_HEADER,  /* header */
        CL_CHAR,    /* regular character */
        CL_ERR,     /* error */
        CL_URL,     /* url */
        CL_EOF,     /* end of file */
        CL_EOL,     /* end of line */
        CL_EOH,     /* end of header */
        CL_VAL,     /* header value */
        CL_COUNT,   /* class count */
};

/* invalid types */
enum {
        LEX_MODE_INV = -1, /* invalid mode (must be first) */
        TT_INV       = -1, /* invalid token */
        CL_INV       = -1, /* invalid class */
};

/* lexer */
struct lex {
        struct iobuf l_buf;                   /* private: io buffer */
        char         l_lex[LEX_LEX_SIZE + 1]; /* private: lexeme */
        char         l_back;                  /* private: putback char */
        int          l_mode;                  /* private: mode */
        int          l_last;                  /* private: last token type */
        int          l_class;                 /* private: token class */
        int          l_type;                  /* private: token type */
};

/**
 * init lex{}:
 *
 * args:
 *  @lp: pointer to lex{}
 *  @fd: file descriptor
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int lex_init(struct lex *lp, int fd);

/**
 * free lex{}:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int lex_free(struct lex *lp);

/**
 * current token type:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: token type
 *  @failure: does not
 */
int lex_type(struct lex *lp);

/**
 * current token class:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: token class
 *  @failure: does not
 */
int lex_class(struct lex *lp);

/**
 * current token type name:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: pointer to token name
 *  @failure: does not
 */
const char *lex_type_name(struct lex *lp);

/**
 * current token class name:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: pointer to class name
 *  @failure: does not
 */
const char *lex_class_name(struct lex *lp);

/**
 * current lexeme:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: pointer to lexeme
 *  @failure: does not
 */
const char *lex_lex(struct lex *lp);

/**
 * move to next token:
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @success: nothing
 *  @failure: does not
 */
void lex_next(struct lex *lp);

/**
 * is lexeme empty?
 *
 * args:
 *  @lp: pointer to lex{}
 *
 * ret:
 *  @true:  if empty
 *  @false: if not
 */
bool lex_empty(const struct lex *lp);

/**
 * move l_buf to r_buf:
 *
 * args:
 *  @lp: pointer to lex{}
 *  @rp: pointer to req{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int lex_buf_move(struct lex *lp, struct req *rp);

#endif /* #ifndef LEX_H */
