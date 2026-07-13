1. **SIGHUP** send conditions
    1. If the ctl-proc exits all fg procs receive SIGHUP
    2. If the ctl-proc loses its terminal the ctl-proc receives SIGHUP
    3. If a pgroup is orphaned with at least one member stopped then each
       member of the pgroup is sent SIGHUP followed by SIGCONT
2. **SIGCHLD** send conditions
    1. At least 1 new wait status becomes available (stopped, exited, continued)

Job run in foreground
    make new bg pgroup for the pipeline
    set the new pgoup to fg group
    add pgroup to job control structure
    block on waitpid until each pipeline member is stopped or exited
    return to foreground
    prompt for next command

Job run in background
    make new bg pgroup for the pipeline
    add pgroup to job control structure
    prompt for next command

