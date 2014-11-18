#pragma once

#include <stdio.h>
#include "database.h"

void smtp(FILE *client, const char *dbfile);
void smtp_handler(FILE *client, void *arg);
