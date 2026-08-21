#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <ncurses.h>

#define PIPE_READ(_pipe, ...) \
    read(_pipe[0], __VA_ARGS__)

#define PIPE_WRITE(_pipe, ...) \
    write(_pipe[1], __VA_ARGS__)

typedef struct {
    WINDOW *win;
    int nlines;
    int ncols;
    int y;
    int x;
} curses_win;

char *flush_pipe(int pipe[2]);
curses_win *init_wins(curses_win wins[4]);

#endif
