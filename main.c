#include "../ctl/lib/include/util.h"
#include "iobuf.h"
#include "lex.h"
#include <stdio.h>
#include <unistd.h>

int
main(void)
{
        struct lex lex = {0};
        int c = -1;

        if (lex_init(&lex, STDIN_FILENO) < 0)
                die("lex_init");

        while ((c = lex_class(&lex)) != CL_ERR && c != CL_EOF) {
                printf("%s: \"%s\"\n", lex_type_name(&lex), lex_lex(&lex));
                lex_next(&lex);
        }

        if (lex_class(&lex) == CL_ERR)
                printf("%s: \"%s\"\n", lex_type_name(&lex), lex_lex(&lex));

        if (lex_free(&lex) < 0)
                die("lex_free");
}
