#ifndef SERV_H
#define SERV_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* server */
struct serv {
        int s_fd; /* listening socket */
};

/**
 * init serv{}:
 *
 * args:
 *  @sp: pointer to serv{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int serv_init(struct serv *sp);

/**
 * free serv{}:
 *
 * args:
 *  @sp: pointer to serv{}
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int serv_free(struct serv *sp);

/**
 * listen:
 *
 * args:
 *  @sp:    pointer to serv{}
 *  @head:  head of addrinfo{} list
 *  @qsize: backlog size
 *
 * ret:
 *  @success: 0
 *  @failure: -1 and errno set
 */
int serv_listen(struct serv *sp, struct addrinfo *head, int qsize);

#endif /* #ifndef SERV_H */
