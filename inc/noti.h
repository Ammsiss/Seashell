#ifndef NOTI_H
#define NOTI_H

#define _GNU_SOURCE

#include "runner.h"

char *get_pid_string(job_id jid);
char *get_cmd_string(job_id jid);

bool noti_jobs(jc_jst *jctl, bool from_sig);

#endif
