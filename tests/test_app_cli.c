#include "app_cli.h"

#include <stdio.h>
#include <string.h>

static int stream_contains(FILE *stream, const char *expected)
{
    char buffer[256];
    size_t bytes_read;

    if (fflush(stream) != 0 || fseek(stream, 0L, SEEK_SET) != 0) {
        return 0;
    }
    bytes_read = fread(buffer, 1U, sizeof(buffer) - 1U, stream);
    buffer[bytes_read] = '\0';
    return strstr(buffer, expected) != NULL;
}

static int reset_stream(FILE *stream)
{
    clearerr(stream);
    return fflush(stream) == 0 && fseek(stream, 0L, SEEK_SET) == 0;
}

int test_app_cli(void)
{
    const char *out_path = "build/test-app-cli-out.tmp";
    const char *err_path = "build/test-app-cli-err.tmp";
    FILE *out = fopen(out_path, "w+b");
    FILE *err = fopen(err_path, "w+b");
    char *help_args[] = {"lab", "--help"};
    char *version_args[] = {"lab", "--version"};
    char *invalid_args[] = {"lab", "--not-a-command"};
    char *extra_args[] = {"lab", "--help", "--version"};
    char *null_arg[] = {"lab", NULL};
    int failed = 0;

    if (out == NULL || err == NULL) {
        (void)fprintf(stderr, "app_cli test failed: temporary stream creation\n");
        if (out != NULL) {
            (void)fclose(out);
        }
        if (err != NULL) {
            (void)fclose(err);
        }
        (void)remove(out_path);
        (void)remove(err_path);
        return 1;
    }

    if (!reset_stream(out) || !reset_stream(err) || app_cli_run(2, help_args, out, err) != 0 ||
        !stream_contains(out, "lab [--help") || !stream_contains(out, "--version")) {
        (void)fprintf(stderr, "app_cli test failed: --help\n");
        failed = 1;
    }
    if (!reset_stream(out) || !reset_stream(err) || app_cli_run(2, version_args, out, err) != 0 ||
        !stream_contains(out, "c-machine-learning-lab 0.1.0")) {
        (void)fprintf(stderr, "app_cli test failed: --version\n");
        failed = 1;
    }
    if (!reset_stream(out) || !reset_stream(err) || app_cli_run(2, invalid_args, out, err) != 2 ||
        !stream_contains(err, "unknown argument: --not-a-command")) {
        (void)fprintf(stderr, "app_cli test failed: unknown option\n");
        failed = 1;
    }
    if (!reset_stream(out) || !reset_stream(err) || app_cli_run(3, extra_args, out, err) != 2 ||
        !stream_contains(err, "--help does not accept additional arguments")) {
        (void)fprintf(stderr, "app_cli test failed: extra arguments\n");
        failed = 1;
    }
    if (!reset_stream(out) || !reset_stream(err) || app_cli_run(2, null_arg, out, err) != 2 ||
        !stream_contains(err, "argument 1 is null")) {
        (void)fprintf(stderr, "app_cli test failed: null argument\n");
        failed = 1;
    }
    if (app_cli_run(1, NULL, out, err) != 2 || app_cli_run(0, NULL, NULL, err) != 2) {
        (void)fprintf(stderr, "app_cli test failed: invalid argument list\n");
        failed = 1;
    }

    if (fclose(out) != 0) {
        (void)fprintf(stderr, "app_cli test failed: closing stdout fixture\n");
        failed = 1;
    }
    if (fclose(err) != 0) {
        (void)fprintf(stderr, "app_cli test failed: closing stderr fixture\n");
        failed = 1;
    }
    if (remove(out_path) != 0) {
        (void)fprintf(stderr, "app_cli test failed: removing stdout stream\n");
        failed = 1;
    }
    if (remove(err_path) != 0) {
        (void)fprintf(stderr, "app_cli test failed: removing temporary streams\n");
        failed = 1;
    }
    return failed;
}
