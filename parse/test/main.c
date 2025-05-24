#include "../../lib/include/util.h"
#include "../../io/include/iobuf.h"
#include "../../http/include/req.h"
#include "../include/lex.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

int
main(void)
{
        struct sockaddr_storage ss = {0};
        struct lex lex = {0};
        struct req req = {0};
        ssize_t n = 0;
        char buf[IOBUF_SIZE + 1];
        int c = -1;

        if (lex_init(&lex, STDIN_FILENO) < 0)
                die("lex_init");

        ss.ss_family = AF_INET;
        if (req_init(&req, &ss) < 0)
                die("req_init");

        while ((c = lex_class(&lex)) != CL_EOF && c != CL_ERR && c != CL_EOH) {
                if (c == CL_METHOD)
                        req_set_method(&req, lex_type(&lex));
                if (c == CL_VERSION)
                        req_set_v(&req, lex_type(&lex));
                lex_next(&lex);
        }

        printf("method:  %s\n", req_method_name(&req));
        printf("version: %s\n", req_v_name(&req));

        if (lex_class(&lex) == CL_ERR) {
                printf("%s: \"%s\"\n", lex_type_name(&lex), lex_lex(&lex));
        } else {
                lex_buf_move(&lex, &req);
                while ((n = req_read(&req, buf, IOBUF_SIZE)) > 0) {
                        buf[n] = 0;
                        printf("%s\n", buf);
                }
        }

        if (lex_free(&lex) < 0)
                die("lex_free");

        if (req_free(&req) < 0)
                die("req_free");
}
