#include "stdio.h"

/*
    The input handler reads a line from the user. It does
    light processing on the line such as replacing tabs
    with spaces.

        char *cmd = io_get_cmd();

    Example:

        FOO=bar echo "$FOO()" | grep "()" > ~/file.txt &

    The lexer then takes raw command input and converts it to
    simple tokens.

        struct lx_tok *list;
        size_t token_list_size;
        lx_tokenize(cmd, &list, &token_list_len);

    Example:

        TOKENS:
            word(FOO=bar)
            word(echo)
            word("$FOO()")
            pipe()
            word(grep)
            word("()")
            rdr_out()
            word(~/file.txt)
            bg()

    The parser groups related tokens into a structural command
    representation. The parser sets up structure for stuff like
    pipelines, subshells, command substitution, setting env
    variables, redirections, semi clolons, etc.

        struct shell_job *job;
        size_t job_list_size
        ps_parse(&job, &job_list_size);

    Example:

        background:
            true
        PIPELINE
            COMMAND
                ASSIGNMENTS:
                    word(FOO=bar)
                WORDS:
                    word(echo)
                    word("$FOO()")
        COMMAND
            REDIRECTS:
                stdout: word(~/file.txt)
            WORDS:
                word(grep)
                word("()")

    The expander analyzes the individual words in the now created
    structural representation and expands them if needed. It is
    concerned with $ (for expansions), basic globbing with *,
    expanding env variables, ~ expansion, command substitution.

    Example:

        background:
            yes
        PIPELINE
            COMMAND
                ASSIGNMENTS:
                    word(FOO=bar)
                WORDS:
                    word(echo)
                    word(()) <- ***
        COMMAND
            REDIRECTS:
                stdout: word(/home/user/file.txt) <- ***
            WORDS:
                word(grep)
                word(())

    The executor now takes the syntactically valid command structure
    and spins up the job(s)
*/

int main(void) {
    printf("hi\n");
}
