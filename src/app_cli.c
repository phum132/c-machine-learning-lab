#include "app_cli.h"

#include <stddef.h>
#include <string.h>

#include "app_commands.h"

#define APP_VERSION "0.1.0"

static void print_usage(FILE *out, const char *program_name)
{
    (void)fprintf(out, "Usage:\n");
    (void)fprintf(out, "  %s [--help | --version]\n", program_name);
    (void)fprintf(out, "  %s train --type linear|logistic --data CSV --target INDEX --model FILE [--header] [--iterations N] [--patience N] [--learning-rate X] [--tolerance X] [--normalize] [--validation-split FRACTION [--seed N]]\n", program_name);
    (void)fprintf(out, "  %s predict --model FILE --input FEATURES.csv [--threshold X]\n", program_name);
    (void)fprintf(out, "  %s evaluate --model FILE --data CSV --target INDEX [--header] [--threshold X]\n", program_name);
    (void)fprintf(out, "  -h, --help     Show this help message\n");
    (void)fprintf(out, "      --version  Show version information\n");
}

int app_cli_run(int argc, char *const argv[], FILE *out, FILE *err)
{
    const char *program_name = "c-machine-learning-lab";

    if (out == NULL || err == NULL) {
        return 2;
    }
    if (argc < 0 || (argc > 0 && argv == NULL)) {
        (void)fprintf(err, "Error: invalid argument list\n");
        return 2;
    }
    if (argc > 0 && argv[0] != NULL && argv[0][0] != '\0') {
        program_name = argv[0];
    }

    if (argc <= 1) {
        print_usage(out, program_name);
        return 0;
    }

    if (argv[1] == NULL) {
        (void)fprintf(err, "Error: argument 1 is null\n");
        return 2;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        if (argc != 2) {
            (void)fprintf(err, "Error: --help does not accept additional arguments\n");
            return 2;
        }
        print_usage(out, program_name);
        return 0;
    }
    if (strcmp(argv[1], "--version") == 0) {
        if (argc != 2) {
            (void)fprintf(err, "Error: --version does not accept additional arguments\n");
            return 2;
        }
        (void)fprintf(out, "c-machine-learning-lab %s\n", APP_VERSION);
        return 0;
    }
    {
        const int result = app_commands_run(argc, argv, out, err);
        if (result == 2) print_usage(err, program_name);
        return result;
    }

}
