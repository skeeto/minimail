#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "database.h"

#define CHECK(db, expr, ret)                                            \
    if ((expr) != SQLITE_OK) {                                          \
        fprintf(stderr, "sqlite3: %s\n", sqlite3_errmsg((db)->db));     \
        return ret;                                                     \
    }

static const char *SQL_TABLE =
    "CREATE TABLE IF NOT EXISTS email"
    "(id INTEGER PRIMARY KEY, user, message)";

static const char *SQL_SEND =
    "INSERT INTO email (user, message) VALUES (?, ?)";

static const char *SQL_LIST =
    "SELECT id, message FROM email WHERE user = ?";

static const char *SQL_DELETE =
    "DELETE FROM email WHERE id = ?";

int database_open(struct database *db, const char *file)
{
    if (sqlite3_open(file, &db->db) != 0)
        return -1;
    CHECK(db, sqlite3_exec(db->db, SQL_TABLE, NULL, NULL, NULL), -1);
    CHECK(db, sqlite3_prepare_v2(db->db, SQL_SEND, -1, &db->send, NULL), -1);
    CHECK(db, sqlite3_prepare_v2(db->db, SQL_LIST, -1, &db->list, NULL), -1);
    CHECK(db, sqlite3_prepare_v2(db->db, SQL_DELETE, -1, &db->del, NULL), -1);
    return 0;
}

int database_close(struct database *db)
{
    return sqlite3_close(db->db);
}

int database_send(struct database *db, const char *user, const char *message)
{
    CHECK(db, sqlite3_reset(db->send), -1);
    CHECK(db, sqlite3_bind_text(db->send, 1, user, -1, SQLITE_TRANSIENT), -1);
    CHECK(db, sqlite3_bind_blob(db->send, 2, message,
                                strlen(message), SQLITE_TRANSIENT), -1);
    return sqlite3_step(db->send) == SQLITE_ROW ? 0 : -1;
}

struct message *
database_list(struct database *db, const char *user)
{
    CHECK(db, sqlite3_reset(db->list), NULL);
    CHECK(db, sqlite3_bind_text(db->list, 1, user, -1, SQLITE_TRANSIENT),
          NULL);
    struct message *messages = NULL;
    while (sqlite3_step(db->list) == SQLITE_ROW) {
        int id = sqlite3_column_int(db->list, 0);
        const void *blob = sqlite3_column_blob(db->list, 1);
        int length = sqlite3_column_bytes(db->list, 1);
        struct message *message = malloc(sizeof(*message) + length);
        memcpy(message->content, blob, length);
        message->id = id;
        message->length = length;
        message->next = messages;
        messages = message;
    }
    return messages;
}

int database_delete(struct database *db, int id)
{
    CHECK(db, sqlite3_reset(db->del), -1);
    CHECK(db, sqlite3_bind_int(db->del, 1, id), -1);
    while (sqlite3_step(db->del) == SQLITE_ROW);
    return 0;
}
