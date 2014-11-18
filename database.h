#pragma once

#include <sqlite3.h>

struct database {
    sqlite3 *db;
    sqlite3_stmt *send, *list, *del;
};

struct message {
    int id;
    int length;
    struct message *next;
    char content[];
};

int database_open(struct database *db, const char *file);
int database_close(struct database *db);

int database_send(struct database *db, const char *user, const char *message);
struct message *database_list(struct database *db, const char *user);
int database_delete(struct database *db, int id);
