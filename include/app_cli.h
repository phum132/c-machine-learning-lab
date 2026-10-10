#ifndef APP_CLI_H
#define APP_CLI_H

#include <stdio.h>

/* Parse command-line arguments, write output to the supplied streams, and
 * return a process exit code. The streams remain owned by the caller. */
int app_cli_run(int argc, char *const argv[], FILE *out, FILE *err);

#endif
