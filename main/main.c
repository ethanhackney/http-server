#include "../serv/include/serv.h"
#include "../lib/include/util.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int
main(void)
{
        struct addrinfo info = {0};
        struct addrinfo *head = NULL;
        struct serv s = {0};
        int e = -1;

        info.ai_family = AF_UNSPEC;
        info.ai_socktype = SOCK_STREAM;
        info.ai_flags = AI_PASSIVE;

        e = getaddrinfo("localhost", "8080", &info, &head);
        if (e != 0)
                die_no_errno("getaddrinfo: %s", gai_strerror(e));

        if (serv_init(&s) < 0)
                die("serv_init");

        if (serv_listen(&s, head, 10) < 0)
                die("serv_listen");

        freeaddrinfo(head);

        if (serv_free(&s) < 0)
                die("serv_free");
}
