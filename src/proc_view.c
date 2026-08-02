#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "dyn_arr.h"
#include "log.h"

#define PPID "PPid"
#define NAME "Name"

#define BUF_SIZE 4096

struct ps_pstat {
    pid_t pid;
    pid_t ppid;
    char name[PATH_MAX];
};

typedef struct ps_pstat ps_pstat;

static char *stat_val_str(pid_t pid, char *field) {
    char stat_path[PATH_MAX];
    snprintf(stat_path, PATH_MAX, "/proc/%d/status", pid);

    FILE *stat = fopen(stat_path, "r");
    if (!stat) {
        LOG_ERRNO("fopen");
        return NULL;
    }

    static char buf[BUF_SIZE];

    char *rv;
    while ((rv = fgets(buf, BUF_SIZE, stat)))
        if (strncmp(field, buf, strlen(field)) == 0)
            break;

    if ((feof(stat) && !rv)) {
        LOG_ERR("field not found\n");
        return NULL;
    }

    if (ferror(stat)) {
        LOG_ERR("error reading from stat");
        return NULL;
    }

    if (fclose(stat) == EOF) {
        LOG_ERRNO("fclose");
        return NULL;
    }

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';

    char *c = strchr(buf, ':');
    if (!c) {
        LOG_ERR("bad stat field\n");
        return NULL;
    }

    for (++c; *c == ' ' || *c == '\t'; ++c)
        continue;

    return c;
}

ps_pstat *lookup_pstat(da_pstat *pstats, char *name) {
    for (size_t i = 0; i < pstats->size; ++i)
        if (strcmp(name, pstats->data[i].name) == 0)
            return &pstats->data[i];

    return NULL;
}

int child_pstat(pid_t pid, da_pstat *pstats) {
    assert(pstats);
    *pstats = (da_pstat){0};

    char child_path[PATH_MAX];
    snprintf(child_path, PATH_MAX, "/proc/%d/task/%d/children", pid, pid);

    FILE *stat = fopen(child_path, "r");
    if (!stat) {
        LOG_ERRNO("fopen");
        return -1;
    }

    static char buf[BUF_SIZE];

    if (!fgets(buf, BUF_SIZE - 1, stat) && feof(stat))
        return 0; /* nochildren */

    if (ferror(stat)) {
        LOG_ERR("ferror");
        return -1;
    }

    if (buf[0] == '\0' || buf[0] == '\n') {
        if (fclose(stat) == EOF) {
            LOG_ERRNO("fclose");
            return -1;
        }

        return 0; /* no children */
    }

    char *endptr = buf;

    while (true) {
        char *start = endptr;

        long child_pid = strtol(start, &endptr, 10);
        if (child_pid == LONG_MAX || child_pid == LONG_MIN) {
            LOG_ERR("strtol overflow");
            return -1;
        }

        if (start == endptr)
            break;

        ps_pstat *pstat = da_push(pstats);
        if (!pstat) {
            LOG_ERR("da_push failed");
            return -1;
        }

        pstat->pid = child_pid;

        char *child_name = stat_val_str(child_pid, NAME);
        if (!child_name)
            return -1;

        strcpy(pstat->name, child_name);
    }

    return 0;
}
