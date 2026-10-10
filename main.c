#include "app_cli.h"

int main(int argc, char *argv[])
{
    return app_cli_run(argc, argv, stdout, stderr);
}
