### Tasks

- [ ] unblock SIGTTOU (any blk/ign sigs) before execing
- [ ] save initial procmask then apply it before execing in child
- [ ] figure out way to automate test file seeing new modules
- [ ] on init failure, the object should be safe to call free on (zero out)
- [ ] set up async handling of SIGCHLD.
- [ ] add regression tests for builtins
- [ ] add err_msg to the fail: label and remove them from syscalls in exec_pline
- [ ] errExit early and use exit handlers to clean up persistant state
- [ ] Find out why we get this output from 'exec valgrind ./run_all.sh'
        ```Warning: ignored attempt to set SIGKILL handler in sigaction();
                the SIGKILL signal is uncatchable
        Warning: ignored attempt to set SIGSTOP handler in sigaction();
                the SIGSTOP signal is uncatchable```
