### Tasks

- [ ] Write a function to parse /proc/stat, find last ) in line to parse comm
- [ ] Add a variadic printf style strcat to dynstr
- [ ] Declarative pty tests that support cc, lines, and escape sequences
- [ ] Write regression tests for the prompt redraw logic
- [ ] PTY harness can have an interactive mode
- [ ] use man 3 backtrace to upgrade logs
- [ ] if we need to test builtins, consider moving jctl back into sh_env
- [ ] add pid string context to job control logs
- [ ] should we do the same meme with job->ev with procs? so like proc->ev
- [ ] on exec_pline failure, excess pfds in parent should be closed

### Unresolved

1. Should we enforce obj init failing means its ok to call obj free?
2. Should functions that mutate state revert the changes on failure?
3. Should we make a light emulation layer so pty tests don't just compare raw
   bytes? For exmpale converting bytes into operations like mv_curs_up(1),
   erase_line(), etc. Then comparing the actual screen state rather then
   just raw bytes being identicle. The pty test harness could then interact
   with the emulator instead for example feed(...) -> cells\[x,y\] = "..."
