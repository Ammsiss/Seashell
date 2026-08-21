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

void curses_err_exit(const char *msg) {
    wattron(wins[HARNESS_WIN].win, COLOR_PAIR(CP_RED_CL));
    wprintw(wins[HARNESS_WIN].win, "ERR ncurses: ");

    wprintw(wins[HARNESS_WIN].win, "%s\n", msg);
    wrefresh(wins[HARNESS_WIN].win);

    wattroff(wins[HARNESS_WIN].win, COLOR_PAIR(CP_RED_CL));
    wattron(wins[HARNESS_WIN].win, CP_DEFAULT);

    exit(EXIT_FAILURE);
}

static XFATAL_HANDLER(xfatal_func) {
    wattron(wins[HARNESS_WIN].win, COLOR_PAIR(CP_RED_CL));
    wprintw(wins[HARNESS_WIN].win, "ERR xfunc: ");

    wprintw(wins[HARNESS_WIN].win, "%s: %s\n", XSYSNAME, strerror(errno));
    wrefresh(wins[HARNESS_WIN].win);

    exit(EXIT_FAILURE);
}

void unity_output_char(int c) {
    if (waddch(wins[UNITY_WIN].win, c) == ERR)
        curses_err_exit("waddch");
    if (wrefresh(wins[UNITY_WIN].win) == ERR)
        curses_err_exit("wrefresh");
}

void llog_output_func(const char *msg, size_t _, llog_lvl lvl) {
    int cp = CP_DEFAULT;

    switch (lvl) {
    case LLOG_INFO: cp = CP_CYAN_CL; break;
    case LLOG_WARN: cp = CP_YELLOW_CL; break;
    case LLOG_ERR:  cp = CP_RED_CL; break;
    }

    wattron(wins[HARNESS_WIN].win, COLOR_PAIR(cp));

    if (wprintw(wins[HARNESS_WIN].win, "%s", msg) == ERR)
        curses_err_exit("wprintw");
    if (wrefresh(wins[HARNESS_WIN].win) == ERR)
        curses_err_exit("wrefresh");

    wattroff(wins[HARNESS_WIN].win, COLOR_PAIR(cp));
    wattron(wins[HARNESS_WIN].win, CP_DEFAULT);
}

static void ncurses_cleanup(void) {
    getch();
    endwin();
}

static void test_init(void) {
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

    if (atexit(ncurses_cleanup) == -1)
        curses_err_exit("atexit");

    set_xfatal_handler(xfatal_func);
    llog_set_output_handler(llog_output_func);
    llog_color_on(false);
    xpipe2(shell_pipe, O_NONBLOCK);
}

static void curses_newwin(curses_win *cwin) {
    cwin->win = derwin(stdscr, cwin->nlines, cwin->ncols, cwin->y, cwin->x);
    if (!cwin->win)
        curses_err_exit("derwin");
    wbkgdset(cwin->win, ' ');
    if (werase(cwin->win) == ERR)
        curses_err_exit("werase");
    if (wrefresh(cwin->win) == ERR)
        curses_err_exit("wrefresh");
}

curses_win *init_wins(curses_win wins[4]) {
    int y, x;
    getmaxyx(stdscr, y, x);

    int by = (y % 2 == 0) ? y / 2 : y / 2 + 1;
    int bx = (x % 2 == 0) ? x / 2 : x / 2 + 1;

    wins[0].nlines = by - 1;
    wins[0].ncols = bx - 1;
    wins[0].y = 0;
    wins[0].x = 0;

    wins[1].nlines = by - 1;
    wins[1].ncols = x / 2;
    wins[1].y = 0;
    wins[1].x = bx;

    wins[2].nlines = y / 2;
    wins[2].ncols = bx - 1;
    wins[2].y = by;
    wins[2].x = 0;

    wins[3].nlines = y / 2;
    wins[3].ncols = x / 2;
    wins[3].y = by;
    wins[3].x = bx;

    for (size_t i = 0; i < 4; ++i) {
        curses_newwin(&wins[i]);
        scrollok(wins[i].win, true);
    }

    return wins;
}

void run_all(void) {
    RUN_TEST_GROUP(prompt);
}

int main(void) {
    test_init();

    int argc = 2;
    const char *argv[] = { "test_pty", "-s", NULL };

    int tests_failed = UnityMain(argc, argv, run_all);

    for (int i = 0; i < 10; ++i) {
        LOG_ERR("this is err %d", i);
        LOG_INFO("this is info %d", i);
        LOG_WARN("this is warn %d", i);
        sleep(1);
    }

    return tests_failed;
}
