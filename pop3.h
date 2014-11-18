#pragma once

#include <stdio.h>
#include "database.h"

void pop3(FILE *client, const char *dbfile);
void pop3_handler(FILE *client, void *arg);
