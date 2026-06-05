#include <stdio.h>

#include "lexer.h"

int main(void) {
    /* display_prompt() */
    /* get_user_input() */

    char *cmd = "echo \"Hello, World!\"";

    da_lx_tok tokens;
    if (lx_tokenize(cmd, &tokens) == -1) {
        fprintf(stderr, "lx_tokenize failed\n");
        exit(EXIT_FAILURE);
    }
}
