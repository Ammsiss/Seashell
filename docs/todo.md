### Tasks

- [ ] Add a test module for the executor/runner (entire shell)

- [ ] should we do the same meme with job->ev with procs? so like proc->ev
- [ ] add an events field to jctl so logs/notis happen after jctl_wait returns
- [ ] Expand asts as needed; simplifies expander and more efficient
- [ ] add a test file for the map structure
- [ ] make all da_delete and other mem failures fatal
- [ ] add -p option to show pids of job members in jobs builtin
- [ ] change PSTOPPED etc to JSTOPPED etc
- [ ] on exec_pline failure, excess pfds in parent should be closed
- [ ] Add a init_proc function and use da_push_init for da_proc
- [ ] Change ps_job -> ps_ast
- [ ] Free any leftover pfds in exec_pline before returning on failure
- [ ] must flush stderr in child subshell before _exit
- [ ] add err_msg to the fail: label and remove them from syscalls in exec_pline
- [ ] errExit early and use exit handlers to clean up persistant state
- [ ] Find out why we get this output from 'exec valgrind ./run_all.sh'
        ```Warning: ignored attempt to set SIGKILL handler in sigaction();
                the SIGKILL signal is uncatchable
        Warning: ignored attempt to set SIGSTOP handler in sigaction();
                the SIGSTOP signal is uncatchable```

### Unresolved

0. should we make jctl_wait() call happen no matter what at the end of the main
   loop? that was anything that changes fgroup doesn't need to wait personally
   just set a flag.
1. Should failed pline child pids be destroyed or added to jctl?
2. figure out way to automate test file seeing new modules
3. How should pre-exec subshell unblocking semantics work?
4. Should we enforce obj_init() failing means its ok to call obj_free()?
5. How should the new executor testing file look with job control?
6. Is it ok to call tcsetpgrp and setpgid on a zombie child?
7. Can we manage array lookups without redoing id logic for each type?
8. On function failure after da_push, should we pop the element before return?
