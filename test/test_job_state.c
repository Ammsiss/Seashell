#include "unity.h"
#include "log.h"
#include "job_state.h"
#include "wait_stat.h"

#define EXP_DA(DA_T, ELEM_T, ...) \
    ((DA_T){ \
        .data = (ELEM_T[]) { __VA_ARGS__ }, \
        .size = sizeof((ELEM_T[]) { __VA_ARGS__ }) / sizeof(ELEM_T) \
     })

/* wait events */

#define WEVS(...) \
    EXP_DA(da_wevent, wait_event, __VA_ARGS__)

#define PCONT(_pid) \
    ((wait_event){ .pid = _pid, .type = PCONTINUED })

#define PSTOP(_pid) \
    ((wait_event){ .pid = _pid, .type = PSTOPPED })

#define PSIG(_pid, _term_sig) \
    ((wait_event){ .pid = _pid, .type = PSIGNALED, .term_sig = _term_sig })

#define PEXIT(_pid, _exit_stat) \
    ((wait_event){ .pid = _pid, .type = PEXITED, .exit_stat = _exit_stat })

#define PEXIT_OK(_pid) \
    ((wait_event){ .pid = _pid, .type = PEXITED, .exit_stat = 0 })

#define PEXIT_FAIL(_pid) \
    ((wait_event){ .pid = _pid, .type = PEXITED, .exit_stat = 1 })

#define STAT(_exit_stat) _exit_stat
#define SIG(_term_sig) _term_sig

/* job events */

#define JEVS(...) \
    EXP_DA(da_jevent, job_event, __VA_ARGS__)

#define JCONT(_jid) \
    ((job_event){ .jid = _jid, .type = JCONTINUED })

#define JSTOP(_jid) \
    ((job_event){ .jid = _jid, .type = JSTOPPED })

#define JEXIT(_jid) \
    ((job_event){ .jid = _jid, .type = JEXITED })

/* jctl structure */

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

#define JRUN_1P(_jid, _pid, ...) \
    JOB(_jid, JOB_RUN, PGRP(_pid, PROC(PID(_pid), PROC_RUN)))

#define JSTOP_1P(_jid, _pid, ...) \
    JOB(_jid, JOB_STOP, PGRP(_pid, PROC(PID(_pid), PROC_STOP)))

#define JID(_jid) _jid
#define PGID(_pgid) _pgid
#define PID(_pid) _pid

#define TABLE(...) \
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
    clear_job_events();
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

void validate_job_events(da_jevent *exp) {
    if (!exp)
        return;

    for (size_t i = 0; i < exp->size; ++i) {
        job_event *jev = pop_job_event();

        TEST_ASSERT_NOT_NULL(jev);
        TEST_ASSERT_EQUAL(exp->data[i].jid, jev->jid);
        TEST_ASSERT_EQUAL(exp->data[i].type, jev->type);
    }

    TEST_ASSERT_NULL(pop_job_event());
}

#define VA_COUNT_I(_1, _2,_3, _4, _5, _6, _7, _8, N, ...) N

#define VA_COUNT(...) \
    VA_COUNT_I(__VA_ARGS__ __VA_OPT__(,) 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define PIDS(...) \
    (da_pid){ .data = (pid_t[]) {__VA_ARGS__}, .size = VA_COUNT(__VA_ARGS__) }

void add_single_proc_job(pid_t jid, pid_t pid) {
    TEST_ASSERT_EQUAL(1, add_job(&PIDS(pid), pid));
    validate_job_table(&TABLE(JRUN_1P(jid, pid)), get_jctl());
    validate_job_events(NULL);
}

void test_pg_leader_missing(void) {
    TEST_ASSERT_EQUAL(-1, add_job(&PIDS(10, 20), 40));
    validate_job_table(&(exp_job_table){0}, get_jctl());
    validate_job_events(NULL);
}

void test_add_job_with_empty_pid_arr(void) {
    TEST_ASSERT_EQUAL(-1, add_job(&(da_pid){ .size = 0 }, 50));
    validate_job_table(&(exp_job_table){0}, get_jctl());
    validate_job_events(NULL);
}

void test_single_proc_job_exit(void) {
    add_single_proc_job(JID(1), PID(1));

    update_job_table(&WEVS(PEXIT_OK(PID(1))));

    validate_job_table(&(exp_job_table){0}, get_jctl());
    validate_job_events(&JEVS(JEXIT(JID(1))));
}

void test_single_proc_job_stop(void) {
    add_single_proc_job(JID(1), PID(1));

    update_job_table(&WEVS(PSTOP(PID(1))));

    validate_job_table(&TABLE(JSTOP_1P(JID(1), PID(1))), get_jctl());
    validate_job_events(&JEVS(JSTOP(JID(1))));
}

void test_single_proc_job_cont(void) {
    add_single_proc_job(JID(1), PID(1));

    update_job_table(&WEVS(PSTOP(PID(1)), PCONT(PID(1))));

    validate_job_table(&TABLE(JRUN_1P(JID(1), PID(1))), get_jctl());
    validate_job_events(&JEVS(JSTOP(JID(1)), JCONT(JID(1))));
}

void test_continue_running_job(void) {
    add_single_proc_job(JID(1), PID(1));

    update_job_table(&WEVS(PCONT(PID(1))));

    validate_job_table(&TABLE(JRUN_1P(JID(1), PID(1))), get_jctl());
    validate_job_events(NULL);
}

void test_non_existant_proc_event(void) {
    TEST_ASSERT_EQUAL_INT(-1, update_job_table(&WEVS(PSTOP(PID(1)))));
}

int main(void) {
    log_init();

    UNITY_BEGIN();

    RUN_TEST(test_pg_leader_missing);
    RUN_TEST(test_add_job_with_empty_pid_arr);
    RUN_TEST(test_single_proc_job_exit);
    RUN_TEST(test_single_proc_job_stop);
    RUN_TEST(test_single_proc_job_cont);
    RUN_TEST(test_continue_running_job);
    RUN_TEST(test_non_existant_proc_event);

    return UNITY_END();
}
