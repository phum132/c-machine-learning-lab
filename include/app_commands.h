#ifndef APP_COMMANDS_H
#define APP_COMMANDS_H

#include <stdio.h>

/* Internal command dispatcher used by app_cli_run. */
int app_commands_run(int argc, char *const argv[], FILE *out, FILE *err);

#endif
