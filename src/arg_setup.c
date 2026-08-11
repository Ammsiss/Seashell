#define _GNU_SOURCE

#include <limits.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <wait.h>
#include <getopt.h>
#include <string.h>

#include "utils.h"
#include "arg_setup.h"

init_data sh_init = {
    .log = {
        .how = LOG_TO_PATH,
        .dir = "/home/juta/Projects/Seashell/logs"
    }
};

int get_int(char *s) {
    if (!s || *s == '\0')
        fatal("get_int: bad arg");

    char *endptr;
    long num = strtol(s, &endptr, 10);

    if (num == LONG_MIN || num == LONG_MAX)
        fatal("strtol");

    if (*endptr != '\0')
        fatal("get_int: non numeric characters");

    return num;
}

void handle_args(int argc, char **argv) {
    int option_index;

    struct option options[3] = {
        { "logfd",  required_argument, NULL, 0 },
        { "logdir", required_argument, NULL, 0 },
        { 0,        0,                0,     0 }
    };

    while (true) {
        char c = getopt_long_only(argc, argv, "", options, &option_index);

        if (c == -1) {
            break;

        } else if (c == 0) {
            if (strcmp("logfd", options[option_index].name) == 0) {
                sh_init.log.how = LOG_TO_FD;
                sh_init.log.fd = get_int(optarg);
            }

        } else if (c == '?')
            fatal("getopt_long_only");
    }
}
