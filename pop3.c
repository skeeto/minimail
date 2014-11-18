#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "database.h"
#include "pop3.h"

void pop3_handler(FILE *client, void *arg)
{
    pop3(client, (const char *) arg);
}

#define RESPOND(client, format, args...)        \
    fprintf(client, format "\r\n", args);

void pop3(FILE *client, const char *dbfile)
{
    struct database db;
    if (database_open(&db, dbfile) != 0) {
        RESPOND(client, "%s", "-ERR");
        fclose(client);
        return;
    } else {
        RESPOND(client, "+OK %s", "POP3 server ready");
    }

    char user[512] = {0};
    struct message *messages = NULL;
    while (!feof(client)) {
        char line[512];
        fgets(line, sizeof(line), client);
        char command[5] = {line[0], line[1], line[2], line[3]};

        if (strcmp(command, "USER") == 0) {
            if (strlen(line) > 5) {
                char *a = line + 5, *b = user;
                while (isalnum(*a))
                    *(b++) = *(a++);
                *b = '\0';
                RESPOND(client, "%s", "+OK");
            } else {
                RESPOND(client, "%s", "-ERR");
            }
            messages = database_list(&db, user);
        } else if (strcmp(command, "PASS") == 0) {
            RESPOND(client, "%s", "+OK"); // don't care
        } else if (strcmp(command, "STAT") == 0) {
            int count = 0, size = 0;
            for (struct message *m = messages; m; m = m->next) {
                count++;
                size += m->length;
            }
            RESPOND(client, "+OK %d %d", count, size);
        } else if (strcmp(command, "LIST") == 0) {
            RESPOND(client, "%s", "+OK");
            for (struct message *m = messages; m; m = m->next)
                RESPOND(client, "%d %d", m->id, m->length);
            RESPOND(client, "%s", ".");
        } else if (strcmp(command, "RETR") == 0) {
            int id = atoi(line + 4);
            for (struct message *m = messages; m; m = m->next)
                if (m->id == id)
                    RESPOND(client, "%s", m->content);
            RESPOND(client, "%s", ".");
        } else if (strcmp(command, "DELE") == 0) {
            int id = atoi(line + 4);
            struct message **last = &messages;
            for (struct message *m = messages; m; m = m->next) {
                if (m->id == id) {
                    *last = m->next;
                    database_delete(&db, id);
                    free(m);
                }
                *last = m->next;
            }
        } else if (strcmp(command, "QUIT") == 0) {
            RESPOND(client, "%s", "+OK");
            break;
        } else {
            RESPOND(client, "%s", "-ERR");
        }
    }

    while (messages) {
        struct message *dead = messages;
        messages = messages->next;
        free(dead);
    }
    fclose(client);
    database_close(&db);
}
