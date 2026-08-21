#include <errno.h>
#include <ncurses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_utils.h"
#include "llog.h"
#include "xfuncs.h"

char *flush_pipe(int pipe[2]) {
    char *out = NULL;
    size_t out_size = 0;

    while (true) {
        char pbuf[4096];
        int num_read = PIPE_READ(pipe, pbuf, 4096);

        if (num_read == -1) {
            if (errno == EAGAIN)
                break;
            else {
                LOG_ERR("read: %m");
                exit(EXIT_FAILURE);
            }
        }

        out = xrealloc(out, out_size + num_read);
        memcpy(out + out_size, pbuf, num_read);
        out_size += num_read;
    }

    out = xrealloc(out, out_size + 1);
    out[out_size] = '\0';

    return out;
}
