#pragma once

#include <pthread.h>

typedef void (*handler_t)(FILE *client, void *arg);

struct server {
    unsigned short port;
    handler_t handler;
    void *arg;
    int fd;
    pthread_t thread;
};

int server_start(struct server *server);
