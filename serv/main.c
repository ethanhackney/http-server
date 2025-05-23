#include "../lib/include/util.h"
#include "../parse/include/iobuf.h"
#include "../parse/include/req.h"
#include "../parse/include/lex.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

/* internal server error */
#define SERV_ERR "HTTP/1.1 500 Internal Server Error\r\n" \
                 "Content-Type: text/plain\r\n"

/**
 * try to listen at host:service with backlog of qsize:
 *
 * args:
 *  @host:  host
 *  @serv:  service (name or port)
 *  @qsize: backlog size
 *
 * ret:
 *  @success: file descriptor
 *  @failure: exit process
 */
static int tcp_listen(const char *host, const char *serv, int qsize);

/**
 * try to listen at address:
 *
 * args:
 *  @ap:    pointer to addrinfo{}
 *  @qsize: backlog size
 *
 * ret:
 *  @success: file descriptor
 *  @failure: -1 and errno set
 */
static int try_addr(const struct addrinfo *ap, int qsize);

/**
 * reap kids:
 *
 * args:
 *  @sig: signal
 *
 * ret:
 *  @success: nothing
 *  @failure: nothing
 */
static void sig_reap(int sig);

/**
 * handle new connection:
 *
 * args:
 *  @fd: client socket
 *  @sp: client address
 *
 * ret:
 *  @success: nothing
 *  @failure: nothing
 */
static void handler(int fd, struct sockaddr_storage *sp);

/**
 * send internal server error response:
 *
 * args:
 *  @fd: socket
 *
 * ret:
 *  @success: nothing
 *  @failure: nothing
 */
static void serv_err(int fd);

/**
 * write buffer to file descriptor:
 *
 * args:
 *  @fd:  descriptor
 *  @buf: buffer
 *  @sz:  buffer size
 *
 * ret:
 *  @success: nothing
 *  @failure: nothing
 */
static void writen(int fd, const void *buf, size_t sz);

int
main(void)
{
        struct sockaddr_storage addr = {0};
        socklen_t addrlen = 0;
        struct sigaction act = {0};
        pid_t pid = 0;
        int servfd = -1;
        int clifd = -1;

        servfd = tcp_listen("localhost", "8080", 10);

        sigemptyset(&act.sa_mask);
        act.sa_handler = sig_reap;
        if (sigaction(SIGCHLD, &act, NULL) < 0)
                die("sigaction");

again:
        addrlen = sizeof(addr);
        clifd = accept(servfd, (struct sockaddr *)&addr, &addrlen);
        if (clifd < 0 && errno == EINTR)
                goto again;
        if (clifd < 0)
                die("accept");

        pid = fork();
        if (pid == 0) {
                if (close(servfd) < 0)
                        die("close servfd");
                handler(clifd, &addr);
                if (close(clifd) < 0)
                        die("close clifd in kid");
                _exit(EXIT_SUCCESS);
        }

        if (pid < 0)
                serv_err(clifd);

        if (close(clifd) < 0)
                die("close clifd in parent");

        goto again;
}

static int
tcp_listen(const char *host, const char *serv, int qsize)
{
        struct addrinfo info = {0};
        struct addrinfo *head = NULL;
        struct addrinfo *ap = NULL;
        int fd = -1;
        int e = -1;

        info.ai_family = AF_UNSPEC;
        info.ai_socktype = SOCK_STREAM;
        info.ai_flags = AI_PASSIVE;

        e = getaddrinfo(host, serv, &info, &head);
        if (e != 0)
                die_no_errno("getaddrinfo: %s", gai_strerror(e));

        for (ap = head; ap != NULL; ap = ap->ai_next) {
                fd = try_addr(ap, qsize);
                if (fd >= 0)
                        break;
        }
        freeaddrinfo(head);
        if (fd < 0)
                die_no_errno("could not listen at %s:%s", host, serv);

        return fd;
}

static int
try_addr(const struct addrinfo *ap, int qsize)
{
        int fd = -1;
        int y = -1;

        fd = socket(ap->ai_family, ap->ai_socktype, ap->ai_protocol);
        if (fd < 0)
                goto ret;

        y = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)) < 0)
                goto close_fd;

        if (bind(fd, ap->ai_addr, ap->ai_addrlen) < 0)
                goto close_fd;

        if (listen(fd, qsize) < 0)
                goto close_fd;

        goto ret;
close_fd:
        if (close(fd) < 0)
                die("close");
        fd = -1;
ret:
        return fd;
}

static void
sig_reap(int sig)
{
        pid_t p = 0;

        while ((p = waitpid(-1, NULL, WNOHANG)) > 0)
                ;

        if (p < 0 && errno != ECHILD)
                die("waitpid");
}

static void
handler(int fd, struct sockaddr_storage *sp)
{
        struct req req = {0};
        struct lex lex = {0};
        int c = -1;

        if (lex_init(&lex, fd) < 0) {
                serv_err(fd);
                return;
        }

        if (req_init(&req, sp) < 0) {
                serv_err(fd);
                goto free_lex;
        }

        while ((c = lex_class(&lex)) != CL_EOF && c != CL_ERR && c != CL_EOH) {
                if (c == CL_METHOD)
                        req_set_method(&req, lex_type(&lex));
                if (c == CL_VERSION)
                        req_set_v(&req, lex_type(&lex));
                if (c == CL_URL)
                        strcpy(req.r_url, lex_lex(&lex));
                lex_next(&lex);
        }
        if (c != CL_EOH) {
                errno = EINVAL;
                serv_err(fd);
                goto free_req;
        }

        printf("method:  %s\n", req_method_name(&req));
        printf("version: %s\n", req_v_name(&req));
        printf("url:     %s\n", req.r_url);

        lex_buf_move(&lex, &req);
        writen(fd, "HTTP/1.1 200 OK\r\n", 17);
        writen(fd, "Content-Length: 12\r\n\r\n", 22);
        writen(fd, "hello world\n", 12);
free_req:
        req_free(&req);
free_lex:
        lex_free(&lex);
}

static void
serv_err(int fd)
{
        const char *errmsg = NULL;
        char res[IOBUF_SIZE + 1] = "";
        int ret = -1;

        errmsg = strerror(errno);
        ret = snprintf(res,
                      sizeof(res),
                      "%s\r\nContent-Length: %zu\r\n\r\n%s\n",
                      SERV_ERR,
                      strlen(errmsg),
                      errmsg);
        if (ret < 0)
                return;

        writen(fd, res, strlen(res));
}

static void
writen(int fd, const void *buf, size_t sz)
{
        const char *p = NULL;
        ssize_t nwrote = -1;
        size_t nleft = 0;

        p = buf;
        nleft = sz;
        while (nleft > 0) {
                nwrote = write(fd, p, nleft);
                if (nwrote < 0 && errno == EINTR)
                        continue;
                if (nwrote < 0)
                        return;
                p += nwrote;
                nleft -= (size_t)nwrote;
        }
}
