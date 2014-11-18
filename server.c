#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server.h"

#define ERROR(format, args...)                                       \
    do {                                                             \
        fprintf(stderr, "%s: " format "\n", strerror(errno), args);  \
        return errno;                                                \
    } while (0)

struct job {
    struct server *server;
    FILE *client;
};

static void *handler_thread(void *arg)
{
    struct job *job = arg;
    job->server->handler(job->client, job->server->arg);
    free(job);
    return NULL;
}

static void *server_thread(void *arg)
{
    struct server *server = arg;
    for (;;) {
        int fd;
        if ((fd = accept(server->fd, NULL, NULL)) == -1) {
            const char *err = strerror(errno);
            fprintf(stderr, "cannot accept connections: %s\n", err);
            return NULL;
        }
        struct job *job = malloc(sizeof(*job));
        job->server = server;
        job->client = fdopen(fd, "r+");
        pthread_t handler;
        if (pthread_create(&handler, NULL, handler_thread, job) != 0) {
            fprintf(stderr, "cannot start client thread\n");
            fclose(job->client);
        } else {
            pthread_detach(handler);
        }
    }
    return NULL;
}

int server_start(struct server *server)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server->port);
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR("%s", "could not create socket");
    if (bind(server->fd, (void *) &addr, sizeof(addr)) != 0)
        ERROR("%s", "could not bind");
    if (listen(server->fd, 1024) != 0)
        ERROR("%s", "could not listen");
    if (pthread_create(&server->thread, NULL, server_thread, server) != 0)
        ERROR("%s", "could create server thread");
    return 0;
}
