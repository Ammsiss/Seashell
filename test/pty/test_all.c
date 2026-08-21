#define _GNU_SOURCE

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#include "unity_fixture.h"
#include "xfuncs.h"
#include "llog.h"
#include "test_utils.h"

#define UNITY_WIN 0
#define HARNESS_WIN 1

#define CP_DEFAULT 0
#define CP_RED_CL 1
#define CP_GREEN_CL 2
#define CP_CYAN_CL 3
#define CP_YELLOW_CL 4

static int shell_pipe[2];
static curses_win wins[4];

#define xwprintw(_win, _fmt, ...) \
    ({ \
        int rv = wprintw(_win, _fmt __VA_OPT__(,) __VA_ARGS__); \
        if (rv == ERR) \
            curs_err("wprintw"); \
        rv; \
    })

#define xwaddch(_win, _ch) \
    ({ \
        int rv = waddch(_win, _ch); \
        if (rv == ERR) \
            curs_err("waddch"); \
        rv; \
    })

#define xwrefresh(_win) \
    ({ \
        int rv = wrefresh(_win); \
        if (rv == ERR) \
            curs_err("wrefresh"); \
        rv; \
    })

#define xderwin(_orig, _nlines, _ncols, _begin_y, _begin_x) \
    ({ \
        WINDOW *rv = derwin(_orig, _nlines, _ncols, _begin_y, _begin_x); \
        if (!rv) \
            curs_err("derwin"); \
        rv; \
    })

#define xwattron(_win, cp) \
    ({ \
        int rv = wattron(_win, cp); \
        if (rv == ERR) \
            curs_err("wattron"); \
        rv; \
    })

#define xwattroff(_win, cp) \
    ({ \
        int rv = wattroff(_win, cp); \
        if (rv == ERR) \
            curs_err("wattroff"); \
        rv; \
    })

#define xwerase(_win) \
    ({ \
        int rv = werase(_win); \
        if (rv == ERR) \
            curs_err("werase"); \
        rv; \
    })

#define wcprintw(_win, cp_num, _fmt, ...) \
    wattron(_win, COLOR_PAIR(cp_num)); \
    wprintw(_win, _fmt __VA_OPT__(,) __VA_ARGS__); \
    wattroff(_win, COLOR_PAIR(cp_num)); \
    wattron(_win, COLOR_PAIR(CP_DEFAULT));

#define xwcprintw(_win, cp_num, _fmt, ...) \
    xwattron(_win, COLOR_PAIR(cp_num)); \
    xwprintw(_win, _fmt __VA_OPT__(,) __VA_ARGS__); \
    xwattroff(_win, COLOR_PAIR(cp_num)); \
    xwattron(_win, COLOR_PAIR(CP_DEFAULT));

static XFATAL_HANDLER(xfatal_func) {
    wcprintw(wins[HARNESS_WIN].win, CP_RED_CL, "ERROR");
    wprintw(wins[HARNESS_WIN].win, " %s: %s\n", XSYSNAME, strerror(errno));

    wrefresh(wins[HARNESS_WIN].win);
    exit(EXIT_FAILURE);
}

static void curs_err(const char *msg) {
    wcprintw(wins[HARNESS_WIN].win, CP_RED_CL, "ERROR");
    wprintw(wins[HARNESS_WIN].win, " ncurses: %s\n", msg);

    wrefresh(wins[HARNESS_WIN].win);
    exit(EXIT_FAILURE);
}

LLOG_SINK(pty_test_sink) {
    int level_cp = CP_DEFAULT;
    char *lvl_str;

    switch (info->log_level) {
    case LLOG_INFO: lvl_str = "INFO";  level_cp = CP_CYAN_CL;   break;
    case LLOG_WARN: lvl_str = "WARN";  level_cp = CP_YELLOW_CL; break;
    case LLOG_ERR:  lvl_str = "ERROR"; level_cp = CP_RED_CL;    break;
    }

    xwcprintw(wins[HARNESS_WIN].win, level_cp, "%s", lvl_str);
    xwprintw(wins[HARNESS_WIN].win, " llog: %s:%d: %s\n",
            info->site->file, info->site->line, info->msg);

    wrefresh(wins[HARNESS_WIN].win);
}

void unity_output_char(int c) {
    xwaddch(wins[UNITY_WIN].win, c);
    xwrefresh(wins[UNITY_WIN].win);
}

curses_win *init_wins(curses_win wins[4]) {
    int y, x;
    getmaxyx(stdscr, y, x);

    int by = (y % 2 == 0) ? y / 2 : y / 2 + 1;
    int bx = (x % 2 == 0) ? x / 2 : x / 2 + 1;

    wins[0] = (curses_win){
        .nlines = by - 1, .ncols = bx - 1,
        .y      = 0,      .x     = 0
    };

    wins[1] = (curses_win){
        .nlines = by - 1, .ncols = x / 2,
        .y      = 0,      .x     = bx
    };

    wins[2] = (curses_win){
        .nlines = y / 2, .ncols = bx - 1,
        .y      = by,    .x     = 0
    };

    wins[3] = (curses_win){
        .nlines = y / 2, .ncols = x / 2,
        .y      = by,    .x     = bx
    };

    for (size_t i = 0; i < 4; ++i) {
        curses_win *cwin = &wins[i];

        cwin->win = xderwin(stdscr, cwin->nlines, cwin->ncols, cwin->y, cwin->x);
        scrollok(wins[i].win, true);
        wbkgdset(wins[i].win, ' ');
        xwerase(cwin->win);
        xwrefresh(cwin->win);
    }

    return wins;
}

static void test_init(void) {
    set_xfatal_handler(xfatal_func);
    llog_set_sink(pty_test_sink);
    xpipe2(shell_pipe, O_NONBLOCK);
}

static void ncurses_cleanup(void) {
    getch();
    endwin();
}

static void ncurses_setup(void) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    start_color();
    use_default_colors();
    init_pair(CP_RED_CL, COLOR_RED, COLOR_BLACK);
    init_pair(CP_GREEN_CL, COLOR_GREEN, COLOR_BLACK);
    init_pair(CP_CYAN_CL, COLOR_CYAN, COLOR_BLACK);
    init_pair(CP_YELLOW_CL, COLOR_YELLOW, COLOR_BLACK);
    bkgdset('*');
    erase();
    refresh();
    init_wins(wins);

    xatexit(ncurses_cleanup);
}

void run_all(void) {
    RUN_TEST_GROUP(prompt);
}

int main(void) {
    ncurses_setup();
    test_init();

    int argc = 2;
    const char *argv[] = { "test_pty", "-s", NULL };

    int tests_failed = UnityMain(argc, argv, run_all);

    return tests_failed;
}
