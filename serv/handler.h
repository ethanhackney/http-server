#ifndef HANDLER_H
#define HANDLER_H

/* handler types */
enum {
        HDLR_POST,  /* POST */
        HDLR_GET,   /* GET */
        HDLR_COUNT, /* handler count */
};

/* invalid types */
enum {
        HDLR_INV = -1, /* invalid handler type */
};

#endif /* #ifndef HANDLER_H */
