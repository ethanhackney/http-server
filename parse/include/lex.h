#ifndef LEX_H
#define LEX_H

#include "../include/iobuf.h"
#include <stdbool.h>

/* misc. constants */
enum {
        LEX_LEX_SIZE = (1 << 10) - 1, /* lexeme size */
        LEX_EOF      = IOBUF_EOF,     /* end of file */
};

/* token types */
enum {
        TT_IO_ERR,     /* io error (must be first) */
        TT_CRLF_ERR,   /* \r not followed by \n */
        TT_BAD_CHAR,   /* invalid character */
        TT_TOO_LONG,   /* lexeme overflow */
        TT_FIRST_BAD,  /* token bad for first line */
        TT_BAD_HDR,    /* bad header */
        TT_PATH,       /* resource path */
        TT_EOF,        /* end of file */
        TT_EOL,        /* end of line */
        TT_EOH,        /* end of headers */
        TT_CHAR,       /* regular character */
        TT_GET,        /* GET */
        TT_V_1_1,      /* HTTP/1.1 */
        TT_HOST,       /* Host */
        TT_USER_AGENT, /* User-Agent */
        TT_ACCEPT,     /* Accept */
        TT_VAL,        /* header value */
        TT_COUNT,      /* token type count */
};

/* token classes */
enum {
        CL_ERR,     /* error (must be first) */
        CL_EOF,     /* end of file */
        CL_EOL,     /* end of line */
        CL_EOH,     /* end of header */
        CL_CHAR,    /* regular character */
        CL_PATH,    /* path */
        CL_METHOD,  /* method */
        CL_VERSION, /* version */
        CL_HEADER,  /* header */
        CL_VAL,     /* header value */
        CL_COUNT,   /* class count */
};

/* lexer */
struct lex {
        struct iobuf l_buf;                   /* private: io buffer */
        char         l_lex[LEX_LEX_SIZE + 1]; /* private: lexeme */
        char         l_back;                  /* private: putback char */
        bool         l_first;                 /* private: on first line? */
        bool         l_hdr;                   /* private: in header name? */
        bool         l_val;                   /* private: in header value? */
        bool         l_pay;                   /* private: in payload? */
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

#endif /* #ifndef LEX_H */
