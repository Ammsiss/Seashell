#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#define NORETURN __attribute__ ((__noreturn__))
#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

NORETURN PFFORMAT(1, 2) void err_exit(const char *fmt, ...)  {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

NORETURN PFFORMAT(1, 2) void usage_err(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "usage: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usage_err("%s [--stop-after STAGE] COMMAND\n", argv[0]);

    dyn_arr list;
    if (lx_tokenize(argv[1], &list) == -1)
        err_exit("lx_tokenize failed\n");

    // print_tok_list(list, size, NULL);

    lx_free(&list);
}
