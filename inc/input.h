#ifndef INPUT_H
#define INPUT_H

#define LINE_BUF 8192

typedef enum {
    INPUT_EOF,
    INPUT_ERR,
    INPUT_SIG,
    INPUT_OK
} input_status;

input_status get_line(char **line);
int display_prompt(void);

#endif
