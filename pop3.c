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
    fprintf(client, format "\r\n", args)

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
        printf("%s", line);
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
            int found = 0;
            for (struct message *m = messages; m; m = m->next)
                if (m->id == id) {
                    found = 1;
                    RESPOND(client, "%s", "+OK");
                    RESPOND(client, "%s", m->content);
                }
            RESPOND(client, "%s", found ? "." : "-ERR");
        } else if (strcmp(command, "DELE") == 0) {
            int id = atoi(line + 4);
            int found = 0;
            for (struct message *m = messages; m; m = m->next) {
                if (m->id == id) {
                    RESPOND(client, "%s", "+OK");
                    database_delete(&db, id);
                    break;
                }
            }
            if (!found)
                RESPOND(client, "%s", "-ERR");
        } else if (strcmp(command, "TOP ") == 0) {
            int id, lines;
            sscanf(line, "TOP %d %d", &id, &lines);
            int found = 0;
            for (struct message *m = messages; m; m = m->next) {
                if (m->id == id) {
                    RESPOND(client, "%s", "+OK");
                    found = 1;
                    char *p = m->content;
                    while (*p && memcmp(p, "\r\n\r\n", 4) != 0) {
                        fputc(*p, client);
                        p++;
                    }
                    if (*p) {
                        p += 4;
                        int line = 0;
                        while (*p && line < lines) {
                            if (*p == '\n')
                                line++;
                            p++;
                        }
                    }
                    break;
                }
            }
            RESPOND(client, "%s", found ? "\r\n." : "-ERR");
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
