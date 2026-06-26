#include "unity.h"
#include "parser.h"
#include "lexer.h"

#define IO_NUM_IN  0
#define IO_NUM_OUT 1
#define IO_NUM_ERR 2

#define APPEND_ON  1
#define APPEND_OFF 0

#define EXP_DA(DA_T, ELEM_T, ...) \
    ((DA_T){ \
        .data = (ELEM_T[]) { __VA_ARGS__ }, \
        .size = sizeof((ELEM_T[]) { __VA_ARGS__ }) / sizeof(ELEM_T) \
     })

#define PLAIN(RAW) \
    ((exp_segment){ .raw = (RAW), .quote = PS_Q_NONE })

#define SEGMENT(RAW, QUOTE) \
    ((exp_segment){ .raw = (RAW), .quote = (QUOTE) })

#define SEGMENTS(...) \
    EXP_DA(exp_da_segment, exp_segment, __VA_ARGS__)

#define WORD(SEGMENTS) \
    ((exp_word){ .segments = (SEGMENTS) })

#define PLAIN_WORD(RAW) \
    WORD(SEGMENTS(PLAIN(RAW)))

#define DQ_WORD(RAW) \
    WORD(SEGMENTS(SEGMENT((RAW), PS_Q_DOUBLE)))

#define WORDS(...) \
    EXP_DA(exp_da_word, exp_word, __VA_ARGS__)

#define NO_REDIRS \
    ((exp_da_redir){0})

#define REDIR(TARGET, IO_NUM, APPEND) \
    ((exp_redir){ .target = (TARGET), .io_num = (IO_NUM), .append = (APPEND) })

#define REDIRS(...) \
    EXP_DA(exp_da_redir, exp_redir, __VA_ARGS__)

#define CMD_R(WORDS, REDIRS) \
    ((exp_cmd){ .words = (WORDS), .redirs = (REDIRS) })

#define CMD(...) \
    ((exp_cmd){ .words = WORDS(__VA_ARGS__), .redirs = NO_REDIRS })

#define SIMPLE_CMD(RAW) \
    CMD(WORDS(PLAIN_WORD(RAW)))

#define CMDS(...) \
    EXP_DA(exp_da_cmd, exp_cmd, __VA_ARGS__)

#define PIPELINE(...) \
    ((exp_pipeline){ .cmds = CMDS(__VA_ARGS__) })

#define NOIF(...) \
    ((exp_andor){ .pipeline = PIPELINE(__VA_ARGS__), .op = PS_NO_IF })

#define ANDIF(...) \
    ((exp_andor){ .pipeline = PIPELINE(__VA_ARGS__), .op = PS_AND_IF })

#define ORIF(...) \
    ((exp_andor){ .pipeline = PIPELINE(__VA_ARGS__), .op = PS_OR_IF })

#define ANDORS(...) \
    EXP_DA(exp_da_andor, exp_andor, __VA_ARGS__)

#define BG_OFF 0
#define BG_ON  1

#define JOB(...) \
    ((exp_job){ .andors = ANDORS(__VA_ARGS__), .bg = BG_OFF })

#define JOB_BG(...) \
    ((exp_job){ .andors = ANDORS(__VA_ARGS__), .bg = BG_ON })

#define JOB_SIMPLE(...) \
    JOB(NOIF(CMD(__VA_ARGS__)))

void setUp(void) {}
void tearDown(void) {}

typedef struct {
    char *raw;
    ps_quote quote;
} exp_segment;

typedef struct {
    exp_segment *data;
    size_t size;
} exp_da_segment;

typedef struct {
    exp_da_segment segments;
} exp_word;

typedef struct {
    exp_word target;
    int io_num;
    int append;
} exp_redir;

typedef struct {
    exp_redir *data;
    size_t size;
} exp_da_redir;

typedef struct {
    exp_word *data;
    size_t size;
} exp_da_word;

typedef struct {
    exp_da_word words;
    exp_da_redir redirs;
} exp_cmd;

typedef struct {
    exp_cmd *data;
    size_t size;
} exp_da_cmd;

typedef struct {
    exp_da_cmd cmds;
} exp_pipeline;

typedef struct {
    exp_pipeline pipeline;
    ps_andor_op op;
} exp_andor;

typedef struct {
    exp_andor *data;
    size_t size;
} exp_da_andor;

typedef struct {
    exp_da_andor andors;
    int bg;
} exp_job;

void validate_segment(const exp_segment *exp, const ps_segment *segment) {
    TEST_ASSERT_EQUAL_STRING(exp->raw, segment->raw);
    TEST_ASSERT_EQUAL(exp->quote, segment->quote);
}

void validate_segments(const exp_da_segment *exp, const da_segment *segments) {
    TEST_ASSERT_EQUAL_size_t(exp->size, segments->size);
    for (size_t i = 0; i < segments->size; ++i) {
        const exp_segment *exp_segment = &exp->data[i];
        const ps_segment *segment = &segments->data[i];
        validate_segment(exp_segment, segment);
    }
}

void validate_tok(const exp_word *exp, const ps_word *word) {
    validate_segments(&exp->segments, &word->segments);
}

void validate_redir(const exp_redir *exp, const ps_redir *redir) {
    TEST_ASSERT_EQUAL_INT(exp->io_num, redir->io_num);
    TEST_ASSERT_EQUAL_INT(exp->append, redir->append);
    validate_tok(&exp->target, &redir->target);
}

void validate_redirs(const exp_da_redir *exp, const da_redir *redirs) {
    TEST_ASSERT_EQUAL_size_t(exp->size, redirs->size);
    for (size_t i = 0; i < redirs->size; ++i) {
        const exp_redir *exp_redir = &exp->data[i];
        const ps_redir *redir = &redirs->data[i];
        validate_redir(exp_redir, redir);
    }
}

void validate_words(const exp_da_word *exp, const da_word *words) {
    TEST_ASSERT_EQUAL_size_t(exp->size, words->size);
    for (size_t i = 0; i < words->size; ++i) {
        const exp_word *exp_tok = &exp->data[i];
        const ps_word *word = &words->data[i];
        validate_tok(exp_tok, word);
    }
}

void validate_cmd(const exp_cmd *exp, const ps_cmd *cmd) {
    validate_words(&exp->words, &cmd->words);
    validate_redirs(&exp->redirs, &cmd->redirs);
}

void validate_cmds(const exp_da_cmd *exp, const da_cmd *cmds) {
    TEST_ASSERT_EQUAL_size_t(exp->size, cmds->size);
    for (size_t i = 0; i < cmds->size; ++i) {
        const exp_cmd *exp_cmd = &exp->data[i];
        const ps_cmd *cmd = &cmds->data[i];
        validate_cmd(exp_cmd, cmd);
    }
}

void validate_pipeline(const exp_pipeline *exp, const ps_pipeline *pipeline) {
    validate_cmds(&exp->cmds, &pipeline->cmds);
}

void validate_andor(const exp_andor *exp, const ps_andor *andor) {
    TEST_ASSERT_EQUAL(exp->op, andor->op);
    validate_pipeline(&exp->pipeline, &andor->pipeline);
}

void validate_andors(const exp_da_andor *exp, const da_andor *andors) {
    TEST_ASSERT_EQUAL_size_t(exp->size, andors->size);
    for (size_t i = 0; i < andors->size; ++i) {
        const exp_andor *exp_andor = &exp->data[i];
        const ps_andor *andor = &andors->data[i];
        validate_andor(exp_andor, andor);
    }
}

void validate_job(const exp_job *exp, const ps_job *job) {
    TEST_ASSERT_EQUAL_INT(exp->bg, job->bg);
    validate_andors(&exp->andors, &job->andors);
}

void validate(const char *shell_cmd, const exp_job *exp) {
    da_tok tokens = {0};
    TEST_ASSERT_NOT_EQUAL_INT(-1, lx_tokenize(shell_cmd, &tokens));

    ps_job job = {0};
    TEST_ASSERT_NOT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    validate_job(exp, &job);

    lx_free(&tokens);
    ps_free(&job);
}

/************ TESTS ************/

void test_bg_not_final_token_should_fail(void) {
    da_tok tokens;  /* Start with 'a' so we don't short circuit */
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize("a & b", &tokens));

    ps_job job;
    TEST_ASSERT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    lx_free(&tokens);
}

void test_redirect_with_no_target_fails(void) {
    da_tok tokens;
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize("echo hi >", &tokens));

    ps_job job;
    TEST_ASSERT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    lx_free(&tokens);
}

void test_pipe_is_final_token_should_fail(void) {
    da_tok tokens;
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize("cmd |", &tokens));

    ps_job job;
    TEST_ASSERT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    lx_free(&tokens);
}

void test_andif_is_final_token_should_fail(void) {
    da_tok tokens;
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize("cmd &&", &tokens));

    ps_job job;
    TEST_ASSERT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    lx_free(&tokens);
}

void test_orif_is_final_token_should_fail(void) {
    da_tok tokens;
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize("cmd ||", &tokens));

    ps_job job;
    TEST_ASSERT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    lx_free(&tokens);
}

void test_no_cmd_redir_should_fail(void) {
    da_tok tokens = {0};
    TEST_ASSERT_EQUAL_INT(0, lx_tokenize("echo > |", &tokens));

    ps_job job = {0};
    TEST_ASSERT_EQUAL_INT(-1, ps_parse(&tokens, &job));

    lx_free(&tokens);
}

void test_bg_is_final_token_should_pass(void) {
    exp_job exp = JOB_BG(
        NOIF(
            CMD(
                PLAIN_WORD("a")
            )
        )
    );
    validate("a &", &exp);
}

void test_cmd_1_word(void) {
    exp_job exp = JOB_SIMPLE(
        PLAIN_WORD("a")
    );
    validate("a", &exp);
}

void test_cmd_2_word(void) {
    exp_job exp = JOB_SIMPLE(
        PLAIN_WORD("a"),
        PLAIN_WORD("b")
    );
    validate("a b", &exp);
}

void test_cmd_many_word(void) {
    exp_job exp = JOB_SIMPLE(
        PLAIN_WORD("command"),
        PLAIN_WORD("arg1"),
        PLAIN_WORD("arg2"),
        PLAIN_WORD("arg3"),
        PLAIN_WORD("arg4"),
    );
    validate("command arg1 arg2 arg3 arg4", &exp);
}

void test_2_cmd_pipeline(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(PLAIN_WORD("a")),
            CMD(PLAIN_WORD("b"))
        )
    );
    validate("a | b", &exp);
}

void test_3_cmd_pipeline(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(PLAIN_WORD("a")),
            CMD(PLAIN_WORD("b")),
            CMD(PLAIN_WORD("c")),
        )
    );
    validate("a | b | c", &exp);
}

void test_2_pipeline_and_if(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(PLAIN_WORD("a"))
        ),
        ANDIF(
            CMD(PLAIN_WORD("b"))
        )
    );
    validate("a && b", &exp);
}

void test_2_pipeline_or_if(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(PLAIN_WORD("a"))
        ),
        ORIF(
            CMD(PLAIN_WORD("b"))
        )
    );
    validate("a || b", &exp);
}

void test_container_control_flow(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(
                PLAIN_WORD("wc"),
                PLAIN_WORD("-l"),
                PLAIN_WORD("file.txt")
            ),
            CMD(
                PLAIN_WORD("grep"),
                PLAIN_WORD("3")
            )
        ),
        ANDIF(
            CMD(
                PLAIN_WORD("echo"),
                DQ_WORD("there are three lines "
                    "in file.txt!")
            ),
        )
    );
    validate("wc -l file.txt | grep 3 && echo \"there are three "
            "lines in file.txt!\"", &exp);
}

void test_and_if_with_redirects(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(PLAIN_WORD("a"))
        ),
        ANDIF(
            CMD_R(
                WORDS(
                    PLAIN_WORD("b")
                ),
                REDIRS(
                    REDIR(
                        PLAIN_WORD("file"),
                        IO_NUM_OUT, APPEND_ON
                    )
                )
            )
        ),
    );
    validate("a && b >> file", &exp);
}

void test_or_if_then_and_if(void) {
    exp_job exp = JOB(
        NOIF(
            CMD(PLAIN_WORD("a"))
        ),
        ORIF(
            CMD(PLAIN_WORD("b")),
        ),
        ANDIF(
            CMD(PLAIN_WORD("c")),
        )
    );
    validate("a || b && c", &exp);
}

void test_redir_in(void) {
    exp_job exp = JOB(
        NOIF(
            CMD_R(
                WORDS(
                    PLAIN_WORD("cat"),
                ),
                REDIRS(
                    REDIR(
                        PLAIN_WORD("input.txt"),
                        IO_NUM_IN, APPEND_OFF
                    )
                )
            )
        )
    );
    validate("cat < input.txt", &exp);
}

void test_redir_out(void) {
    exp_job exp = JOB(
        NOIF(
            CMD_R(
                WORDS(
                    PLAIN_WORD("ls"),
                ),
                REDIRS(
                    REDIR(
                        PLAIN_WORD("file.txt"),
                        1, 0
                    )
                )
            )
        )
    );
    validate("ls > file.txt", &exp);
}

void test_redir_err(void) {
    exp_job exp = JOB(
        NOIF(
            CMD_R(
                WORDS(
                    PLAIN_WORD("solaar"),
                    PLAIN_WORD("show"),
                ),
                REDIRS(
                    REDIR(
                        PLAIN_WORD("/dev/null"),
                        IO_NUM_ERR, APPEND_OFF
                    )
                )
            )
        )
    );
    validate("solaar show 2> /dev/null", &exp);
}

void test_redir_append(void) {
    exp_job exp = JOB(
        NOIF(
            CMD_R(
                WORDS(
                    PLAIN_WORD("ping"),
                    PLAIN_WORD("1.1.1.1")
                ),
                REDIRS(
                    REDIR(
                        PLAIN_WORD("log.txt"),
                        IO_NUM_OUT, APPEND_ON
                    )
                )
            )
        )
    );
    validate("ping 1.1.1.1 >> log.txt", &exp);
}

void test_multiple_redirects(void) {
    exp_job exp = JOB(
        NOIF(
            CMD_R(
                WORDS(
                    PLAIN_WORD("syslog"),
                ),
                REDIRS(
                    REDIR(
                        PLAIN_WORD("output"),
                        IO_NUM_OUT, APPEND_OFF
                    ),
                    REDIR(
                        PLAIN_WORD("output2"),
                        IO_NUM_OUT, APPEND_OFF
                    ),
                    REDIR(
                        PLAIN_WORD("errors"),
                        IO_NUM_ERR, APPEND_OFF
                    ),
                )
            )
        )
    );
    validate("syslog > output > output2 2> errors", &exp);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bg_not_final_token_should_fail);
    RUN_TEST(test_redirect_with_no_target_fails);
    RUN_TEST(test_no_cmd_redir_should_fail);
    RUN_TEST(test_pipe_is_final_token_should_fail);
    RUN_TEST(test_andif_is_final_token_should_fail);
    RUN_TEST(test_orif_is_final_token_should_fail);

    RUN_TEST(test_bg_is_final_token_should_pass);

    RUN_TEST(test_cmd_1_word);
    RUN_TEST(test_cmd_2_word);
    RUN_TEST(test_cmd_many_word);

    RUN_TEST(test_2_cmd_pipeline);
    RUN_TEST(test_3_cmd_pipeline);

    RUN_TEST(test_2_pipeline_and_if);
    RUN_TEST(test_2_pipeline_or_if);
    RUN_TEST(test_or_if_then_and_if);
    RUN_TEST(test_container_control_flow);
    RUN_TEST(test_and_if_with_redirects);

    RUN_TEST(test_redir_in);
    RUN_TEST(test_redir_out);
    RUN_TEST(test_redir_err);
    RUN_TEST(test_redir_append);
    RUN_TEST(test_multiple_redirects);

    return UNITY_END();
}
