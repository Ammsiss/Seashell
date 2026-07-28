#include "unity.h"
#include "log.h"
#include "job_state.h"

#define EXP_DA(DA_T, ELEM_T, ...) \
    ((DA_T){ \
        .data = (ELEM_T[]) { __VA_ARGS__ }, \
        .size = sizeof((ELEM_T[]) { __VA_ARGS__ }) / sizeof(ELEM_T) \
     })

#define PROC(_pid, _stat) \
    ((exp_proc){ .pid = _pid, .stat = _stat })

#define PROCS(...) \
    EXP_DA(exp_da_proc, exp_proc, __VA_ARGS__)

#define PGRP(_pgid, ...) \
    ((exp_pgrp){ .pgid = _pgid, .procs = PROCS(__VA_ARGS__) })

#define JOB(_jid, _stat, _pgrp) \
    ((exp_job){ .jid = _jid, .stat = _stat, .pgrp = _pgrp })

#define JOBS(...) \
    EXP_DA(exp_da_job, exp_job, __VA_ARGS__)

#define JID(_jid) _jid
#define PGID(_pgid) _pgid
#define PID(_pid) _pid

#define JOB_TABLE(...) \
    ((exp_job_table){ .jobs = JOBS(__VA_ARGS__) })

typedef struct {
    pid_t pid;
    proc_stat stat;
} exp_proc;

typedef struct {
    exp_proc *data;
    size_t size;
} exp_da_proc;

typedef struct {
    pid_t pgid;
    exp_da_proc procs;
} exp_pgrp;

typedef struct {
    pid_t jid;
    exp_pgrp pgrp;
    job_stat stat;
} exp_job;

typedef struct {
    exp_job *data;
    size_t size;
} exp_da_job;

typedef struct {
    exp_da_job jobs;
} exp_job_table;

struct job_event {
    pid_t jid;
    job_event_type type;
};

void setUp(void) {}

void tearDown(void) {
    clear_job_table();
}

void validate_proc(exp_proc *exp, jc_proc *proc) {
    TEST_ASSERT_EQUAL(exp->pid, proc->pid);
    TEST_ASSERT_EQUAL(exp->stat, proc->stat);
}

void validate_pgrp(exp_pgrp *exp, jc_pgrp *pgrp) {

    TEST_ASSERT_EQUAL(exp->pgid, pgrp->pgid);
    TEST_ASSERT_EQUAL_size_t(exp->procs.size, pgrp->procs.size);

    for (size_t i = 0; i < pgrp->procs.size; ++i)
        validate_proc(&exp->procs.data[i], &pgrp->procs.data[i]);
}

void validate_job(exp_job *exp, jc_job *job) {

    TEST_ASSERT_EQUAL(exp->jid, job->jid);
    TEST_ASSERT_EQUAL(exp->stat, job->stat);
    TEST_ASSERT_NOT_NULL(job->last);

    validate_pgrp(&exp->pgrp, &job->pgrp);
}

void validate_job_table(exp_job_table *exp, job_table *jctl) {

    TEST_ASSERT_EQUAL_size_t(exp->jobs.size, jctl->jobs.size);

    for (size_t i = 0; i < jctl->jobs.size; ++i)
        validate_job(&exp->jobs.data[i], &jctl->jobs.data[i]);
}

#define JEV(_jid, _type) \
    (job_event){ .jid = _jid, .type = _type }

#define JEVS(...) \
    (job_event []){__VA_ARGS__}

void validate_job_events(job_event *exp, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        job_event *jev = pop_job_event();

        TEST_ASSERT_NOT_NULL(jev);
        TEST_ASSERT_EQUAL(exp[i].jid, jev->jid);
        TEST_ASSERT_EQUAL(exp[i].type, jev->type);
    }

    TEST_ASSERT_NULL(pop_job_event());
}

#define VA_COUNT_I(_1, _2,_3, _4, _5, _6, _7, _8, N, ...) N

#define VA_COUNT(...) \
    VA_COUNT_I(__VA_ARGS__ __VA_OPT__(,) 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define PIDS(...) \
    (da_pid){ .data = (pid_t[]) {__VA_ARGS__}, .size = VA_COUNT(__VA_ARGS__) }

void test_add_job(void) {
    TEST_ASSERT_EQUAL(JID(1), add_job(&PIDS(1), PGID(1)));
    TEST_ASSERT_EQUAL(JID(2), add_job(&PIDS(301, 305, 400), PGID(301)));
    TEST_ASSERT_EQUAL(JID(3), add_job(&PIDS(8080, 9000, 9050, 9999), PGID(8080)));

    exp_job_table exp_table = JOB_TABLE(
        JOB(JID(1), JOB_RUN,
            PGRP(PGID(1),
                PROC(PID(1), PROC_RUN)
            )
        ),
        JOB(JID(2), JOB_RUN,
            PGRP(PGID(301),
                PROC(PID(301), PROC_RUN),
                PROC(PID(305), PROC_RUN),
                PROC(PID(400), PROC_RUN),
            )
        ),
        JOB(JID(3), JOB_RUN,
            PGRP(PGID(8080),
                PROC(PID(8080), PROC_RUN),
                PROC(PID(9000), PROC_RUN),
                PROC(PID(9050), PROC_RUN),
                PROC(PID(9999), PROC_RUN)
            )
        )
    );

    validate_job_table(&exp_table, get_jctl());
    validate_job_events(NULL, 0);
}

void test_pg_leader_missing(void) {
    TEST_ASSERT_EQUAL(-1, add_job(&PIDS(2), 1));
    TEST_ASSERT_EQUAL(-1, add_job(&PIDS(10, 20, 30), 40));
    TEST_ASSERT_EQUAL(-1, add_job(&PIDS(100, 900, 400), 50));

    exp_job_table exp_table = (exp_job_table){0};

    validate_job_table(&exp_table, get_jctl());
    validate_job_events(NULL, 0);
}

void test_add_job_with_empty_pid_arr(void) {
    TEST_ASSERT_EQUAL(-1, add_job(&(da_pid){ .size = 0 }, 50));

    exp_job_table exp_table = (exp_job_table){0};

    validate_job_table(&exp_table, get_jctl());
    validate_job_events(NULL, 0);
}

int main(void) {
    log_init();

    UNITY_BEGIN();

    RUN_TEST(test_add_job);
    RUN_TEST(test_pg_leader_missing);
    RUN_TEST(test_add_job_with_empty_pid_arr);

    return UNITY_END();
}
