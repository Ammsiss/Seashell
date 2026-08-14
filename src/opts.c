#include "opts.h"
#include "args.h"

opt_data opts[] = {
    [LOGFD] = {
        .long_name = "logfd",
        .short_name = 'l',
        .has_arg = REQUIRED_ARG,
        .arg_type = INT_ARG
    },
    [LOGDIR] = {
        .long_name = "logdir",
        .short_name = 'p',
        .has_arg = REQUIRED_ARG,
        .arg_type = STR_ARG
    },
    (opt_data){0}
};
