#ifndef INPUT_H
#define INPUT_H

#define LINE_BUF 8192

typedef enum {
    INPUT_EOF,
    INPUT_ERR,
    INPUT_OK
} input_stat;

input_stat get_line(char **line);
void display_prompt(void);

#endif
