#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "smtp.h"

void smtp_handler(FILE *client, void *arg)
{
    smtp(client, (const char *) arg);
}

#define RESPOND(client, code, message)          \
    fprintf(client, "%d %s\r\n", code, message)

struct recipient {
    char email[512];
    struct recipient *next;
};

void recipient_push(struct recipient **rs, char *email)
{
    struct recipient *r = malloc(sizeof(*r));
    char *p;
    for (p = r->email; *email && *email != '@'; p++, email++)
        *p = *email;
    *p = '\0';
    r->next = *rs;
    *rs = r;
}

struct recipient *recipient_pop(struct recipient **rs)
{
    struct recipient *r = *rs;
    if (*rs)
        *rs = (*rs)->next;
    return r;
}

void smtp(FILE *client, const char *dbfile)
{
    struct database db;
    if (database_open(&db, dbfile) != 0) {
        RESPOND(client, 421, "database error");
        fclose(client);
        return;
    } else {
        RESPOND(client, 220, "localhost");
    }

    char from[512] = {0};
    struct recipient *recipients = NULL;
    while (!feof(client)) {
        char line[512];
        fgets(line, sizeof(line), client);
        char command[5] = {line[0], line[1], line[2], line[3]};

        if (strcmp(command, "HELO") == 0) {
            RESPOND(client, 250, "localhost");

        } else if (strcmp(command, "MAIL") == 0) {
            strcpy(from, line);
            RESPOND(client, 250, "OK");

        } else if (strcmp(command, "RCPT") == 0) {
            if (!from[0]) {
                RESPOND(client, 503, "bad sequence");
            } else if (strlen(line) < 12) {
                RESPOND(client, 501, "syntax error");
            } else {
                recipient_push(&recipients, line + 9);
                RESPOND(client, 250, "OK");
            }

        } else if (strcmp(command, "DATA") == 0) {
            RESPOND(client, 354, "end with .");
            size_t size = 4096, fill = 0;
            char *content = malloc(size);
            for (;;) {
                fgets(line, sizeof(line), client);
                if (strcmp(line, ".\r\n") == 0)
                    break;
                if (strlen(line) + fill >= size) {
                    size *= 2;
                    content = realloc(content, size);
                }
                strcpy(content + fill, line);
                fill += strlen(line);
            }
            while (recipients) {
                struct recipient *r = recipient_pop(&recipients);
                database_send(&db, r->email, content);
                free(r);
            }
            free(content);
            RESPOND(client, 250, "OK");

        } else if (strcmp(command, "QUIT") == 0) {
            RESPOND(client, 221, "localhost");
            break;

        } else {
            RESPOND(client, 500, "command unrecognized");
        }
    }
    fclose(client);
    database_close(&db);
}
