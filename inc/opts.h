#ifndef OPTS_H
#define OPTS_H

#include "args.h"

typedef enum {
    LOGFD,
    LOGDIR,
} opt_names;

extern opt_data opts[];

#endif
