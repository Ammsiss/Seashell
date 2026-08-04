### Tasks

- [ ] if request_job_id returns different, need to update fg_jid
- [ ] investigate input followed by sigint followed by enter valgrind issue
- [ ] consider making x-funcs err_exit directly on failure
- [ ] make pop_job_event not use static storage
- [ ] factor out lookup logic in run_next_job_in_plan
- [ ] test stopping andor chain with plines left
- [ ] stress test request_job_id with many different jobs exiting
- [ ] make exec_pline use an outparam for pline_data
- [ ] move fg_event() to shell_stat.c
- [ ] Add a ncurses debug display
- [ ] Intead of exiting with failure on sighup, self kill with sighup
- [ ] Declarative pty tests that support cc, lines, and escape sequences
- [ ] Write regression tests for the prompt redraw logic
- [ ] PTY harness can have an interactive mode
- [ ] use man 3 backtrace to upgrade logs
- [ ] if we need to test builtins, consider moving jctl back into sh_env
- [ ] add pid string context to job control logs
- [ ] on exec_pline failure, excess pfds in parent should be closed
- [ ] consider only expanding 1 andor chain at a time etc
- [ ] consider allowing add_jobs to free pline_data

### Unresolved

1. Should we enforce obj init failing means its ok to call obj free?
2. Should functions that mutate state revert the changes on failure?
3. Should we make a light emulation layer so pty tests don't just compare raw
   bytes? For exmpale converting bytes into operations like mv_curs_up(1),
   erase_line(), etc. Then comparing the actual screen state rather then
   just raw bytes being identicle. The pty test harness could then interact
   with the emulator instead for example feed(...) -> cells\[x,y\] = "..."
4. Is fg_jid enough to replace the subshell boolean?
5. Is the simpler void * dyn array strictly just better?
