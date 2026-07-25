#ifndef WAITSTAT_H
#define WAITSTAT_H

#include <sys/wait.h>

#include "dyn_arr.h"

struct wait_event {
    pid_t pid;
    int wstat;
};

typedef struct wait_event wait_event;

void get_wstats(da_wevent *wevs);

#endif
