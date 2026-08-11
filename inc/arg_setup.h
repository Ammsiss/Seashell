#ifndef ARG_SETUP_H
#define ARG_SETUP_H

#define LOG_TO_PATH 0
#define LOG_TO_FD 1

typedef struct {
    int how;
    union {
        char *dir;
        int fd;
    };
} log_init;

typedef struct {
    log_init log;
} init_data;

extern init_data sh_init;

void handle_args(int argc, char **argv);

#endif
