---------------------------------------------------------------------------------

## 2026-07-9

### Next

- [ ] Investigate why tcsetpgrp is failing

### Tasks

- [ ] handle SIGTTIN in child
- [ ] parent should report STOPPED and CONTINUED through waitpid
- [ ] add err_msg to the fail: label and remove them from syscalls in exec_pline

**Complete**
- [x] refactor exec_pline

### Notes

Job control is pretty interesting. Just re-reading some material on it
and testing the waters for now. Already ran into a head scratcher but It
shouldn't be too bad. Why is tcsetpgrp failing... I don't think its the
fd not refering to the controlling terminal but the pgrp thing should also
be right. Good luck tommorow me.

---------------------------------------------------------------------------------

## 2026-07-8

### Next

- [ ] Job control!

### Tasks

Where is everyone?

**Complete**
- [x] err_exit should always exit with EXIT_FAILURE, use _exit where needed
- [x] use statfs to discern pipes from fifos in validate_pfd_count
- [x] add happy path tests for redirections
- [x] set builtin should not allow key to have ' '
- [x] set builtin should not allow key to have any symbols besides _ or -
- [x] add da_delete function for the st_unset_var function
- [x] write unset builtin
- [x] add a separate source file for the builtins.
- [x] move out error checking from buitins into "try run builtin"
- [x] add arg handling to exit builtin
- [x] Use X macros to centralize da_array registration
- [ ] (Removed) set builtin should not allow duplicate key entries
- [ ] (Removed - requires expander overhaul) Expand unquoted ~ with $HOME
- [ ] (Removed) use xda_arr log helpers but for shell modules
- [ ] (Removed) Add logging of file descriptors using proc/pid/fd
- [ ] (Removed) Come up with a system for toggleable tracing.

### Notes

Lots done today, sort of just checking stuff off. Moving the builtins to
their own source was a nice change, the api it actually needs to expose
turned out to be tiny (just try_run_builtin()) so the executor got a lot
simpler.

I don't think proactively writing trace code makes sense after thinking about
it. It should probably naturaly develop from just debugging areas of code where
you write print statements to check assumptions then over time if those print
statements keep being useful you build some structure around it so that they
stay up and you don't start from zero every time. But the shell is still simple
enough that debugging still feels pretty surface level. No need for permanent
traces yet.

Choosing the boring todo items and just sticking with them seems to pay rent
much more then following interesting little rabbit holes. Going to try and do
that more. I got a suprising amount done today just by choosing a todos.

When you choose a todo and then another good next step arises from working on
it, I think its better to not do it right away. Write it down and finish the
current item even if it would just be deleted/reverted by the next step
anyways. I think its better because you don't want to stack changes, you start
the current todo with some assumptions and if mid way through the todo you move
on to another todo you discovered in the half finished state of the first todo
its as lot easier to forget the initial assumptions and break something. It
feels really tempting to just do it quickly becuase it seems directly related
but its not. Its it's own unique todo that you may or may not want to do after.

---------------------------------------------------------------------------------

## 2026-07-7

### Next

- [ ] err_exit should always exit with EXIT_FAILURE, use _exit where needed
- [ ] add happy path tests for redirections

### Tasks

**Expander**
- [ ] set builtin should not allow key to have ' '
- [ ] set builtin should not allow key to have any symbols besides _ or -
- [ ] set builtin should not allow duplicate key entries

**Misc**
- [ ] Expand unquoted ~ with $HOME
- [ ] Use X macros to centralize da_array registration
- [ ] Consider recreating the xda_arr log helpers but for shell modules
- [ ] Add logging of file descriptors using /proc/pid/fd...
- [ ] Come up with a system for toggleable tracing.

**Complete**
- [x] Remove all xsyscall macros. logs aren't meant to be hidden
- [x] Add macro wrappers for logs that preserve context macros
- [x] Add a validation func for fd count in exec_pipeline
- [x] Write redir expansion function
- [x] Add initial redirecion implementation in executor

### Notes

All planned features are done! Except job control of course. But redirections,
variable expansion, pipelines, and andor chains are all implemented. I think I'm
going to spend the next week cleaning up, adding tests, and scoping out how
job control will be implemented.

Also pondering adding a script mode. At the simplest level its as simple as
just swapping the input source from lines from the interactive getline() loop
to a small binary parsing lines from a script file.

Control structures like if statements and loops wouldn't actaully be that hard
to implement at the executor level. It would just be one or multiple andor
chains with meta data describing how many times to run them in order. No need
to make a boolean system either, you can just use the return status of the
commands so if statements and boolean logic is pretty simple too.

---------------------------------------------------------------------------------

## 2026-07-6

### Next
- [ ] Remove all xsyscall macros. logs aren't meant to be hidden
- [ ] Write expand redir function

### Tasks

**Expander**
- [ ] set builtin should not allow key to have ' '
- [ ] set builtin should not allow key to have any symbols besides _ or -
- [ ] set builtin should not allow duplicate key entries

**Misc**
- [ ] Expand unquoted ~ with $HOME
- [ ] Use X macros to centralize da_array registration
- [ ] Consider recreating the xda_arr log helpers but for shell modules

**Complete**
- [x] Add an API func sh_run so valgrind can see memory issues during tests
- [x] Write regression tests for variable logic.
    - [x] $FOO -> 'bar'
    - [x] \$FOO -> '$FOO'
    - [x] $FOOword -> '' (assuming FOOword is not a real key)
    - [x] $FOO zoo -> 'bar zoo'
    - [x] bee$FOO -> 'beebar'
    - [x] $FOO\bee -> 'barbee'
    - [x] $FOO$FOO -> 'barbar' (doesn't work)
    - [x] $FOO\$FOO -> 'bar$FOO' (doesn't work)
- [x] (Done a while ago) Expander removes double quotes based on quote mode
- [x] (Done a while ago) Expander builds arg value for ps_redir
- [ ] (Removed) Write a map data structure.
- [ ] (Removed - scope) Add generic io num tokens

### Notes

Wen't down a pretty unproductive rabbit hole trying to refactor and simplify
exec_pipeline. Had a whole state structure tracking the saved pfd and
everything but I ended up scrapping it after like 2 hours. Oh well. At least I
understand what doesn't work better.

Made some last minute progress when I actually just stuck to the task list! You
can now specify the input and output fd's for sh_run so valgrind can see them
during tests and we got all the new regression tests for the expander
implemented and passing + no valgrind errors.

---------------------------------------------------------------------------------

## 2026-07-5

### Next
- [ ] Add an API func sh_run so valgrind can see memory issues during tests
- [ ] Write regression tests for variable logic.
    - [ ] $FOO -> 'bar'
    - [ ] \$FOO -> '$FOO'
    - [ ] $FOOword -> '' (assuming FOOword is not a real key)
    - [ ] $FOO zoo -> 'bar zoo'
    - [ ] bee$FOO -> 'beebar'
    - [ ] $FOO\bee -> 'barbee'
    - [ ] $FOO$FOO -> 'barbar' (doesn't work)
    - [ ] $FOO\$FOO -> 'bar$FOO' (doesn't work)

### Tasks

**Expander**
- [ ] Expander removes double quotes based on quote mode
- [ ] Expander builds arg value for ps_redir
- [ ] set builtin should not allow key to have ' '
- [ ] set builtin should not allow key to have any symbols besides _ or -
- [ ] set builtin should not allow duplicate key entries

**Misc**
- [ ] Logs don't make line number sense. calling LOG in xclose is bad
- [ ] Write a map data structure. Perhaps using binary tree already wrote
- [ ] Expand unquoted ~ with $HOME
- [ ] Add generic io num tokens so redirects can apply to any fd num
- [ ] Use X macros to centralize da_array registration
- [ ] Consider recreating the xda_arr log helpers but for shell modules

**Complete**
- [x] Create a dynamic string module. s->data\[s->len\] == '\0' invariant

### Notes

Expansion started. Looking real important to have the executor regression tests
run under valgrind. A whole new suite of funcs which depend pretty heavily on
valgrind (variable expansion tests) are now needed and basically every feature
from here on out will be tested through the executor. So thats the next thing
im doing.

The dynamic string mod was much simpler then I thought. I decided to just
literally figure out exactly what was needed in the expander and just implement
that. It ended up being a dedicated strcat function, and a character push
function. The rest is basically just a copy from the dynamic array module. Of
course it gets more useful if you can do insertions, deletions, appends, and
you can do them with both char * and 2 dynamic strings. Also certain algorithms
as nice to have's like str.reverse(), str.compare(), etc.

One thing I'm unsure of is the "detach behaviour". The c_str is allways
malloc'd and a lot of the time its nice to be able to just return the c_str
from a function but it feels a bit weird to call d_str_init() and not
d_str_free(). It's fine but it might point to needing a dedicated detach
function (which I opted against for now becuase it felt like a whole bunch of
ceramony for no real gain) or just using the dynamic string everywhere. (or
just more places).

---------------------------------------------------------------------------------

## 2026-07-4

### Next
- [ ] Create a dynamic string module. s->data\[s->len\] == '\0' invariant

### Tasks

**Expander**
- [ ] Expander removes double quotes based on quote mode
- [ ] Expander builds arg value for ps_redir
- [ ] set builtin should fail if argv(1) has any whitespace

**Misc**
- [ ] Write a map data structure. Perhaps using binary tree already wrote
- [ ] Add an API func sh_run so valgrind can see memory issues during tests
- [ ] Expand unquoted ~ with $HOME
- [ ] Add generic io num tokens so redirects can apply to any fd num
- [ ] Use X macros to centralize da_array registration
- [ ] Consider recreating the xda_arr log helpers but for shell modules

**Complete**
- [x] Create utils.c/h files for the err(x) funcs and common macros
- [x] Start basic lexer error status return value
- [x] Add shell variable support with set builtin. just storage no usage yet
- [x] Start expander variable parsing
- [ ] (Removed - bad coupling) Lexer/Parser calls err_msg function
- [ ] (Removed - more specific todo added) Add redirection handling

### Notes

Decided to not add logs to the lexer and parser modules. Seemed like annoying
coupling especially because the api is just 2 functions each and called at only
1 place. I think better to remove the dependency and just return a status code
and have 1 more func to return a static struct with more error information if
needed.

Realized I definitely need a dynamic string module in order to not go crazy
writing the expander. The expander has variable length segments, words, args,
expansions that can result in longer, shorter, equal length strings, it has to
do concats, insertions, deletions.. you get the picture.

I also am going to need a map data structure for shell variables. At least the
set builtin was dead simple to implement becuase the parser already handles
what counts as a word for the 

---------------------------------------------------------------------------------

## 2026-07-3

### Next
- [ ] Create utils.c/h files for the err(x) funcs and common macros
- [ ] Lexer/Parser calls the err_msg function

### Tasks

- [ ] Add an API func sh_run so valgrind can see memory issues during tests
- [ ] Add redirection handling
- [ ] Start expander variable parsing
- [ ] Expand unquoted ~ with $HOME
- [ ] Add generic io num tokens so redirects can apply to any fd num
- [ ] Use X macros to centralize da_array registration

**Complete**
- [x] In the fork loop, store and track pids you create and wait specifically
- [x] Factor out wait loop and wait only for the pids you create

### Notes

Good day today, had those curtains installed so kind of distracted. They
are pretty nice tho.

---------------------------------------------------------------------------------

## 2026-07-2

### Next

- [ ] In the fork loop, store and track pids you create and wait specifically
- [ ] Create utils.c/h files for the err(x) funcs and common macros
- [ ] Lexer/Parser calls the err_msg function

### Tasks

- [ ] Add an API func sh_run so valgrind can see memory issues during tests
- [ ] Add redirection handling
- [ ] Start expander variable parsing
- [ ] Expand unquoted ~ with $HOME
- [ ] Add generic io num tokens so redirects can apply to any fd num

**Complete**
- [x] Move argv creation responsibility from executor to expander
- [x] Add -c mode so you can capture shell output for regression tests
- [x] Add andor and pipeline regression tests
- [x] Builtin paths implemented for parent and sub shells
- [x] Refactor sh_run from switch to if statements
- [ ] (Removed) Executor diagnostic module for interacting with the shell
- [ ] (Removed - obvious) Job control
- [ ] (Removed - obvious) Add switch case to the loop based on quote level

### Notes

Some good simplifications today. Don't need the convoluted sh_return structure,
I just return the exit status of the last pipe or on parent shell failure I
print a errmsg and return -1.

Also after seeing that sh also just nukes the process image and gets lots of
"still reachable" leaks in valgrind I very quickly adopted that method as well.
Much simpler then trying to set up weird exit handlers or pass variables around
just to free something thats going to be nuked anyways.

Last but not least, I think I'm done with switch statements except for very
obvious uses like maps or enum resolution. Everytime I use them for more
logical stuff It ends up being less readable and better implemented with if
statements.

---------------------------------------------------------------------------------

## 2026-07-1

### Next
- [ ] Move argv creation responsibility from executor to expander
- [ ] Add -c mode so you can capture shell output for regression tests

### Tasks

**Diagnostics**
- [ ] Lexer/Parser diagnostics for known failure paths at least
- [ ] Executor diagnostic module for interacting with the shell

**Executor**
- [ ] Add redirection handling
- [ ] Job control

**Expander**
- [ ] Add switch case to the loop based on quote level
- [ ] Expand unquoted ~ with $HOME

**Complete**
- [x] Create helper functions for the pipeline wiring logic
- [x] In run_pipeline only create a pipe if there is a next command
- [x] Rename read_fd and pfd to make more sense semantically
- [x] Refactor the pipeline->cmds.size == 1 path

### Notes

Good day today. Simplified the run_pipeline structure a lot. While working with
the pipeline algo I was recommended to use a truth table and after writing it
out I saw the use pretty clearly. Formalizing this type of logic is probably
pretty good for efficiency so I'm going to start a discrete math text after
geometry!

---------------------------------------------------------------------------------

## 2026-06-30

### Next

- [ ] In run_pipeline only create a pipe if there is a next command
- [ ] Rename read_fd and pfd to make more sense semantically
- [ ] Create helper functions for the pipeline wiring logic
- [ ] Refactor the pipeline->cmds.size == 1 path

Think about what it means logically for the size == 1 case to be handled
generically. It just means its a pipe with no input or output fds.

LOL after looking at it again im pretty it ALREADY handles size == 1 Well this
task should be easy then.

### Tasks

**Diagnostics**
- [ ] Lexer/Parser diagnostics for known failure paths at least
- [ ] Executor diagnostic module for interacting with the shell

**Executor**
- [ ] Add redirection handling
- [ ] Job control

**Expander**
- [ ] Add switch case to the loop based on quote level
- [ ] Expand unquoted ~ with $HOME

**Complete**
- [x] Remove the LOG_EXIT style functions in favor of explicit _exit calls
- [x] Simplify the log_msg function; just use static storage
- [x] exit builtin "proof of concept"

### Notes

log_msg function way simpler. Just using static storage for the fields
is much more sane.

Starting to sus out the needed structure for builtins. A query function that
can check argv(0) and return some sort of code is going to be my first play.

---------------------------------------------------------------------------------

## 2026-06-29

### Next

- [ ] Simplify the log_msg function; just use static storage
- [ ] Remove the LOG_EXIT style functions in favor of explicit _exit calls

### Tasks

**Diagnostics**
- [ ] Lexer/Parser diagnostics for known failure paths at least
- [ ] Executor diagnostic module for interacting with the shell

**Executor**
- [ ] Add redirection handling
- [ ] Job control

**Expander**
- [ ] Add switch case to the loop based on quote level
- [ ] Expand unquoted ~ with $HOME

**Complete**
- [x] Remove all unnecessary identifier prefixes on static types
- [x] Replace the log_info/err etc functions cause macros do their job
- [x] Add log_exit style functions as a convenience
- [x] (Started) wrap system calls to add trace level log messages automatically
- [ ] (Removed) system call wrappers can have define checks for toggling tracing

### Notes

Good day today, made some good progress with logging/diagnostics. A big
takeaway for the day would be that after a fork the shell basically treats the
sub shell as any other command in a pipeline. Theres a little pre exec
procedure but regardless of if it fails before the exec, on the exec, or after,
the shell treats it the same.

Failed before the exec? The subshell just prints to stderr (like any other
program) then terminates. The parent shell doesn't care.

Exec failed? The subshell just prints to stderr and exits with 127, again the
parent shell doesn't need to know the details of how you failed just if you did
or not.

Initially I was overcomplicating it becuase I was thinking that if you had a
"shell" error like the subshell exec call failing I thought there would have to
be some special clean up or the shell would need to cancel the rest of the
pipeline or something. But no, even if it does fail before the exec it works
out fine. The next program that tries to read stdin would just see EOF because
the subshell doesn't write anything to the pipe and then closes immediately.

Besides there is no way to differentiate the subshell exiting before the exec
and retruning 127 and the exec suceeding where the execced program returns 127
anyways.

So the subshell basically just becomes another program in the pipeline,
printing to stderr if it encounters any issues.

---------------------------------------------------------------------------------

## 2026-06-28

### Next

- [ ] Replace the log_info/err etc functions cause macros do their job
- [ ] Add log_exit style functions as a convenience
- [ ] Add test file for regression tests

### Tasks

**Logging**
- [ ] wrap system calls to add trace level log messages automatically
- [ ] system call wrappers can have define checks for toggling tracing

**Diagnostics**
- [ ] Lexer/Parser diagnostics for known failure paths at least
- [ ] Executor diagnostic module for interacting with the shell

**Executor**
- [ ] Add redirection handling
- [ ] Job control

**Expander**
- [ ] Add switch case to the loop based on quote level
- [ ] Expand unquoted ~ with $HOME

**Complete**
- [x] Record exit status of each pipeline for andor logic
- [x] Implement && and || handling
- [x] run_pipeline reimplemented iteratively
- [x] Simple logging module
- [x] Use tail -F plus a sym link to the latest session log
- [x] The log_msg function should call write once so its atomic
- [x] Remove all unnecessary identifier prefixes on static types
- [ ] (Removed) Investigate very bad clangd performance on test_parser.c
- [ ] (Removed) Use --error-markers option to color valgrind output
- [ ] (Removed) Rewrite the run_all.sh script to use a shell function
- [ ] (Removed) Parse out "PASS" lines from the run_all.sh script
- [ ] (Removed) Block valgrind output if any tests fail
- [ ] (Removed) Write a visualizer for lexer and parser
- [ ] (Removed) Start a diagnostics module
- [ ] (Removed) Parse for $ and expand any relevant env variables
- [ ] (Removed) Replace \n with a real newline in arg
- [ ] (Removed) Change the file names for dyn_arr, maybe dynarr
- [ ] (Removed) Maybe add explicit parenthesis to ALL macro args no matter what

### Notes

Things are picking up! The data structures I toiled over seem to be proving
their worth. && and || was pretty trivial and redirecitons shouldn't be too
bad.

Moving into a repl style debugging work flow now. Hand crafted tests are too
slow (Will still write regression tests tho). The feed back is much faster with
interactive shell sessions + real time logs.

See pics/first_repl.png for an example repl session at this stage.

The (Removed) items in the task list just means that they either weren't
relevant enough for the current state of the project and they were just adding
noise to the task list or were solved inadvertently and made a non issue.

---------------------------------------------------------------------------------

## 2026-06-27

### Next

- [ ] Implement && and || handling.

### Tasks

**Logging**
- [ ] The log_msg function should call write once so its atomic
- [ ] Use tail -F plus a sym link to the latest session log
- [ ] Add log_exit equivalents
- [ ] Replace the log_info/err etc functions cause macros do their job
- [ ] investiage why C-d to command 'cat' causes no waitpid logs
- [ ] wrap system calls to add trace level log messages automatically
- [ ] system call wrappers can have define checks for toggling tracing

**Diagnostics**
- [ ] Lexer/Parser diagnostics for known failure paths at least

**Executor**
- [ ] Set up any relevant redirections
- [ ] Add test file for regression tests
- [ ] Record exit status of each pipeline for andor logic

**Expander**
- [ ] Parase for $ and expand any relevant env variables
- [ ] Expand unquoted ~ with $HOME
- [ ] Replace \n with a real newline in arg

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c
- [ ] Use --error-markers option to color valgrind output
- [ ] Rewrite the run_all.sh script to use a shell function
- [ ] Parse out "PASS" lines from the run_all.sh script
- [ ] Block valgrind output if any tests fail

**Misc**
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Write a tree view helper (Maybe generic?)
- [ ] Start a diagnostics module
- [ ] Maybe add explicit parenthesis to ALL macro args no matter what
- [ ] Change the file names for dyn_arr, maybe dynarr

**Complete**
- [x] Hook up procceses in pipelines before execing

### Notes

Pipelines now work. Realizing I'm going to need a logging module now that
forking is on the table. Might look into debugging multi process programs.
pics/milestone2.png shows a pipeline exec.

Should forked shells return -1 on failure if they fail before exec?

How can we have diagnostics for forked shells that have already set their fd's
in a pipeline? In other words they don't have access to the terminals stdout
anymore.

Why doesn't ./seashell cat receive C-d?

---------------------------------------------------------------------------------

## 2026-06-26

### Next

- [ ] Hook up procceses in pipelines before execing

### Tasks

**Executor**
- [ ] Set up any relevant redirections
- [ ] Record exit status of each pipeline for andor logic

**Expander**
- [ ] Parase for $ and expand any relevant env variables
- [ ] Expand unquoted ~ with $HOME

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c
- [ ] Use --error-markers option to color valgrind output
- [ ] Rewrite the run_all.sh script to use a shell function
- [ ] Parse out "PASS" lines from the run_all.sh script
- [ ] Block valgrind output if any tests fail

**Misc**
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Write a tree view helper (Maybe generic?)
- [ ] Start a diagnostics module
- [ ] Maybe add explicit parenthesis to ALL macro args no matter what
- [ ] Change the file names for dyn_arr, maybe dynarr

**Complete**
- [x] Replace integer boolean with the actual boolean type
- [x] In spec.md write out how redirects should work
- [ ] (Removed) Fill out the rough skeleton of the mod/parser.md contract

### Notes

MILESTONE! All the layers are created and we can officially go from a shell
command to an execed program! The executor is just running commands and not
doing any pipeline hookups, andor logic, or redirections but its nice to
see the core flow working. See pics/milestone1.png for a test run.

Expander should parse redirects and just apply the fd dup rule naively so
later redirects are the one you end up with if multiple are provided.

---------------------------------------------------------------------------------

## 2026-06-25

### Next

- [ ] Replace integer boolean with the actual boolean type

**Start**: `kind_is_delim()` and `kind_is_double_char_op()` should both return
`true` or `false` not `0` or `1`.

**Context**: Simply better readability.

**Complete**: All appropriate variables changed to bool instead
of int and all literals changed to true and false.

### Tasks

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c
- [ ] Use --error-markers option to color valgrind output
- [ ] Rewrite the run_all.sh script to use a shell function
- [ ] Parse out "PASS" lines from the run_all.sh script
- [ ] Block valgrind output if any tests fail

**Documentation**
- [ ] Fill out the rough skeleton of the mod/parser.md contract
- [ ] In spec.md write out how redirects should work

**Misc**
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Write a tree view helper (Maybe generic?)
- [ ] Start a diagnostics module
- [ ] Maybe add explicit parenthesis to ALL macro args no matter what
- [ ] Change the file names for dyn_arr, maybe dynarr

**Complete**
- [x] _Generic interface (part 2) used at all call sites
- [x] Make helper macro for the push then init pattern
- [x] Silence the "static function not used in TU" nonsense
- [x] Change `malloc()` for `calloc()` wherever it makes sense

### Notes

Tried to fix the unused header warning in parser.c due to IWYU seeing that we
only use the typedef from lexer_types.h and incorrectly concluding that the
definition in lexer.h is not needed, even though we preform member access which
does require the full definition.

This is not an architectural problem just a result of IWYU style ownership
being quite opinionated and strict. Going forward I'm just going to silence
warnings of this class.

The flow going forward will be if you only need types you include mod_types.h.
If you only need definitions you include mod.h. If you wanted to be more
explicit you could also include the mod_types.h file when you need definitions
(because you would be using the typedefs regardless) but for the sake of
brevity I won't.

dyn_arr.c is no longer C portable because of the addition of the GNU statement
expression macro.

The da_push_init helper has a nice benefit of enforcing the memory contract.
Any struct that can manage dynamic memory should be set to a known state before
use. This helper combines those 2 operations so its harder to forget.

---------------------------------------------------------------------------------

## 2026-06-24

### Next

- [ ] _Generic interface (part 2) used at all call sites

**Start**: Add the second generic macro to call the selected function.

**Context**: We just made the generic interface, its tested and
in a nice spot. We just haven't actually replaced all the typed
calls yet, so this is the easy part!

**Completion**: All relevant calls replaced with the new generic
macro function.

### Tasks

**Dynamic Array**
- [ ] Make helper macro for the `da_push` + init pattern

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c
- [ ] Silence the "static function not used in TU" nonsense
- [ ] Use --error-markers option to color valgrind output
- [ ] Rewrite the run_all.sh script to use a shell function
- [ ] Parse out "PASS" lines from the run_all.sh script
- [ ] Block valgrind output if any tests fail

**Documentation**
- [ ] Fill out the rough skeleton of the mod/parser.md contract
- [ ] In spec.md write out how redirects should work

**Misc**
- [ ] Change `malloc()` for `calloc()` wherever it makes sense
- [ ] Replace integer boolean with the actual boolean type
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Debug dump tree view helper (Maybe generic?)
- [ ] Start a diagnostics module
- [ ] Centralize clang build flags (like -gnu23 so its easier to change)

**Complete**:
- [x] _Generic interface for da_type functions

### Notes

Figured out a way to fix the circular dependency (with dyn_arr.h and
the module headers) that arised when I realized that the _Generic selector
needed to see all the declarations and types of the dynamic arrays.

At first the problem was that I was trying to use typedef statements as forward
declarations but that doesn't work cause I had duplicate typedefs in the module
headers.

So my idea was to add only the typedef statements themselves into header files
so that the dyn_arr.h file could include it while the module can still include
the dyn_arr.h file.

As a consequence the new contract is that dyn_arr types are centralized.
Registration requires a DEFINE macro invocation in dyn_arr.c and a DECLARE
macro invocation in dyn_arr.h.

May be worth also creating a dyn_arr_types.h file as well and move the typedef
in the DECLARE macro into something like a DEFINE_DYN_ARR_TYPE macro to be
consistent with the other modules but it seems needless for now.

---------------------------------------------------------------------------------

## 2026-06-23

### Next

- [ ] _Generic interface for da_type functions

**Start**: Remove all the scattered DEFINE_DYN_ARR macro invocations,
then for each one, add an invocation of DECLARE_DYN_ARR in dyn_arr.h
and DEFINE_DYN_ARR in dyn_arr.c. Then compile and see if the new structure
is working.

**Context**: We're currently in the process of adding the _Generic interface
for the dyn_arr but the current method of defining an array in one shot where
needed (like da_cmd in parser.h) won't work because for a file to call the
generic interface it needs to know about each possible type it can resolve too
as well as the functions declarations.

We created a declare macro that has the declarations for each function as well
as moved the struct definition into it. So the contract going forword will be
for every new dyn arr that's needed we invoke the DECLARE macro in dyn_arr.h
and the DEFINE in dyn_arr.c. That allows each file to invoke the generic
interface without needing to include entire modules.

**Completetion**: Generic dynamic array interface added, tests
added.

### Tasks

**Dynamic Array**
- [ ] Make helper functions for the `da_push` + init pattern

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c
- [ ] Silence the "static function not used in TU" nonsense
- [ ] Use --error-markers option to color valgrind output
- [ ] Rewrite the run_all.sh script to use a shell function
- [ ] Parse out "PASS" lines from the run_all.sh script
- [ ] Block valgrind output if any tests fail

**Documentation**
- [ ] Fill out the rough skeleton of the mod/parser.md contract
- [ ] In spec.md write out how redirects should work

**Misc**
- [ ] Change `malloc()` for `calloc()` wherever it makes sense
- [ ] Replace integer boolean with the actual boolean type
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Debug dump tree view helper (Maybe generic?)
- [ ] Start a diagnostics module

**Complete**
- [x] Factor out the growth pattern in `da_push()` into `da_reserve()`

### Notes

The refactor felt clean, the interface seems simpler and theres no longer any
malloc special case after realizing that realloc with a null pointer is just
the same as an equivalent malloc call.

da_init can no longer fail (it just zeroes out the struct) but I will still
check for -1 returns to stay consistent with the rest of the init functions.

Came up with the supposedly good _Generic pattern myself which was cool.
Basically you define 2 macros: One for function selection only and another for
making the actual call with the resolved function. The key takeaway being that
even if a type case is never selected the resolution still has to be valid C.

---------------------------------------------------------------------------------

## 2026-06-22

### Next

- [ ] Factor out the growth pattern in `da_push()` into `da_reserve()`

**Start**: Write the `da_reserve()` function. After you do that you can
remove the initial capacity parameter from the da_init functions.

**Context**: Removing the initial capacity paramter is nice because in every
place I call the init function im passing 0 anyways. But I'm not just removing
the functionality the user will still be able to call the `da_reserve()`
function (after `da_init()`) if they need to. The same function that the
`da_push()` will be calling, which seems like good consistency.

**Completion**: `da_reserve()` function written, core unit tests written
and passed, `da_push()` calls it, and initial capacity function in da_init
removed.

### Tasks

**Dynamic Array**
- [ ] _Generic interface for da_type functions
- [ ] Make helper functions for the `da_push` + init pattern

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c
- [ ] Silence the "static function not used in TU" nonsense
- [ ] Use --error-markers option to color valgrind output
- [ ] Rewrite the run_all.sh script to use a shell function
- [ ] Parse out "PASS" lines from the run_all.sh script
- [ ] Block valgrind output if any tests fail

**Documentation**
- [ ] Fill out the rough skeleton of the mod/parser.md contract
- [ ] In spec.md write out how redirects should work

**Misc**
- [ ] Change `malloc()` for `calloc()` wherever it makes sense
- [ ] Replace integer boolean with the actual boolean type
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Debug dump tree view helper (Maybe generic?)
- [ ] Start a diagnostics module

**Complete**
- [x] Add init and free functions for `lx_part`
- [x] da_init with size 0 should not allocate any memory
- [x] Fix the potential overflow bug in da_push
- [x] Remove direct child pipeline from ps_job

### Notes

Good progress day, we actually touched some code this time.

Straightened out the memory contract in the lexer and parser and made them more
consistent. Once I implement the _Generic interface for the dynamic arrays
macrotizing the push + init pattern should be much easier as the caller wont
have to pass in the typed push function just its init function. Also its just
cleaner.

Removed the pipeline from ps_job after realizing theres not really a point in
having it there. I already had a PS_NO_IF enum too which is the perfect use for
the first andor in the list. That change seemed to have some positive effects
on my test structure creation macros too. I was able to remove some nesting
that was making it harder to parse. Also I made helper macros with names
indicating the extra functionality that they have instead of just making the
user specify them as a macro arguments. It seems much nicer to just call
JOB_BG(ANDIF(...)) isntead of JOB_BG(ANDIF(...), BG). Its especially noticable
at scale. If they end up having a lot more options of course it might create a
lot of combinations though, we'll see!

I'm also thinking about writing a generic tree view printer. A bunch of times
I've wanted to print a representation of a data structure like a binary tree
PID/PPID relationship graph, or now with the lexer and parser structures and
reimplementing it every time is pretty sucky. This might be a decently big
project though or it could definitely succomb to scope creep. But the core of
it should be pretty simple. To start I could literally just have it print with
indenting, no fancy box characters showing parent child relationships. A nice
touch would be allowing the caller to specify labels for the root and children
at level N of the tree. Also there could be higher level specifications like
saying that child X is an array, which causes it to automatically print out the
children with an array index tag or something.

Starting to feel the pain of running unity tests raw with no script helpers.
Finally started to feel painful enough that I made a little run_all.sh script.
Very simple but its started to open my eyes on how much cleaner I can get the
output and how much easier it can be to parse. Plus it automates the valgrind
checks which can be easy to forget and annoying to do manually. Testing is looking
like something kind of 2 pass. You run your logical test first, if they pass
then you run valgrind if it passes, golden. If logical tests fail, no need to
continue because at least in this case my clean up functions don't run anyways
cause unity asserts immediately so its just noise. So in the script im going to
parse out valgrind output if the unity tests failed.

For readability of the valgrind and unity output i'm definitely going to parse
out PASS lines from unity and also completely silence valgrinds non error
output. For valgrind its easy just pass -q but unity requires a bit more manual
work.

Valgrind has a cool error marker option that might make highlighting relevant
fields easier for example the line numbers in the function stack and file.
Seems like another sidequest though cause I've never liked shell scripting.

Im wondering if making every structures deafult "safe" state a zeroed out
struct could be a good simplification. Instead of always needing explicit
init functions you could simply zero out the struct on declarations and thats
an explicit default state. For example with the dyn array zeroing it out would
set the data pointer to NULL and the size and cap to 0. Thats basically the
empty array state no need to call the init function unless you need a initial
capacity.

The main drawback would be potential inconsistencies with sometimes initializing
by zeroing out the struct or calling init. If a struct for some reason needed
a init call you could run into issues. For now I'm going to stick with explicit
initialization + free calls no matter, seems simpler.

---------------------------------------------------------------------------------

## 2026-06-21

### Next

- [ ] Add init and free functions for `lx_part`
- [ ] da_init with size 0 should not allocate any memory

**Start**: with adding init/free pair for the lx_part function and then of
course replace the inline init/free with the functions.

**Context**: In `add_part` we call `da_part_push` but do not immediately call
init like we should.

M8 should be a simple change, just adjust the init function and then run tests
with valgrind.

**Completion**: After every da_push there is an init call. For every init call
there is a free call.

### Tasks

**Parser**
- [ ] Make the 'first' pipeline a ps_andor with PS_NO_IF to simplify executor

**Tooling**
- [ ] Investigate very bad clangd performance on test_parser.c

**Documentation**
- [ ] Fill out the rough skeleton of the mod/parser.md contract
- [ ] In spec.md write out how redirects should work

**Misc**
- [ ] Change `malloc()` for `calloc()` wherever it makes sense
- [ ] Silence the "static function not used in TU" nonsense
- [ ] Replace integer boolean with the actual boolean type
- [ ] Remove all unnecessary identifier prefixes on static types
- [ ] Add a _Generic interface for da_type functions
- [ ] Make helper functions for the `da_push` + init pattern

**Complete**
- [x] Remove lx_tok coupling by using a different word structure in the ast
- [x] Updated AST data structure by adding intermediary ps_pipeline struct
- [x] All init and free functions in parser zero out the structures
- [x] All init functions in parser assert on NULL input pointer

### Notes

Good day today, not much code changes but a lot of good leads on potential
refactors and architectural changes.

Memory allocation! I'm starting to realize that having a consistent contract
with memory management is pretty important. Every time I saw a little deviation
or special case it slowed me down. It made me have to reason about how it
worked and why it was needed.

For example in my `add_tok` function in lexer.c I had a special case with
initializing tokens. I was initializing `tok.parts` only if `tok.kind ==
LX_TOK_WORD`. It made sense initially because I didn't have a init or free
function for `lx_tok` so before I added them I was initializing it inline. The
pain point was that there was no clean way to free the token without either
checking if it was a word before calling free, or making sure that if it
wasn't the relevant fields were are out.

This made me appreciate the simple convention of always having a init and free
function for ANY structure that owns dynamic memory at any point. Even if the
init function does not actually ever initialize dynamic memory its still useful
because you can at that point, put it in a known state (for example zeroing it
out) such that calling free is OK.

Implementing the parser helped me see this more clearly because its data
structure is basically a scaled up version of the token array. I could get away
with scattered "code local" conventions and special cases because it was easier
to reason about.

In general I'm starting to appreciate more and more how crucial conventions,
consistency, and readability is. That is basically what programming is, a pile
of conventions and norms that you agree on that allows you to cache and
compress the representation of your project. If your project doesn't have much
structure then every day you add more code the less you will understand because
its another unique pattern that you will have to re-intuit each time you visit
that part of the code base. That destroys momentum too. If you build good
foundations and conventions then you can start to trust them. Things deviating
from the standard will stand out, not be the norm.

---------------------------------------------------------------------------------

## 2026-06-20

### Next

1. Complete P2.

**Start**: Think about what the simplest structure could be because it
obviously does not need the "kind" member. The AST already gives that
information.

**Completion**: The new structure is integrated into the parser, a init and
free function is added for it, the lx_free_tok() function is static as it is
no longer needed outside of the lexer, and all tests pass.

### Tasks

**Parser**
- [ ] P1: Make the 'first' pipeline a ps_andor with PS_NO_IF to simplify executor
- [ ] P2: Remove lx_tok coupling by using a different word structure in the ast

**Misc**
- [ ] M1: Change malloc() for calloc() wherever it makes sense
- [ ] M2: Silence the "static function not used in TU" nonsense.
- [ ] M3: Replace integer boolean with the actual boolean type

### Completed

- Created a docs/ directory for documentation files including: module
  contracts, a project log (what your reading right now) and an overall spec
  for the shell.

### Notes

Felt a bit uneasy today because I'm taking a bit of a detour to work on
documentation and a more structured setup for my work sessions but I think its
the right play. I'm realizing that I've always had very little structure for
personal projects which might be the reason they become frustrating or
derailed. Other skills I work on are much more structured. Starting is almost
instant. Geometry? Just do the next question. Piano? Do the predefined time
blocks that you set up. Programming needs that structure too. It's no
different.

The on and off ramp for each session should be engineered in a particular way.
It should be super easy to start. Perhaps a predefined on ramp like a quick win
or a low risk task that gets you started and reduces avoidance. Then having a
closure routine like writing a project log and preparing the next day's entry
task gives you permission to stop working and feel accomplished.

I've also been thinking about ways to tie in documentation with the "test
driven development" I've sort of been doing. There is definitely some synergy
there... Anyways I'll end this with some of my thoughts on the benefits of
documentation and testing. One obvious and one less so.

I think there are 2 main benefits of documentation.

- First is the more obvious benefit that you have an externalized expectation
  of what your modules should guarantee which probably makes it easier to know
  what tests you should write and what might not actually make sense.
- Another benefit is for consistency. Documentation gives you a productive
  outlet to solidify your understanding of the project without necessarily
  having to add features. This is a pretty non threatening type of work because
  your not risking any regression. You also get physical proof of progress as
  your documentation grows which makes the work feel real.

For testing I think it's a similar idea.

- First you have the mental bandwidth reduction of not constantly having to be
  aware of potential regressions in your changes. You just do it, then run the
  test suite and at least you can be decently confident if you wrote your tests
  well that nothing broke. Your tests also become a sort of documentation
  support. Your API contract defines the boundaries and your tests make it
  real.
- The less obvious benefit is again for consistency and emotional regulation!
  Tests give you a bit of built in structure in regards to the "what next"
  question. You can always write another test. Writing tests feels like a win.
  Its a bit more proof that your code works. It has a cool little green color
  in the pass output. That tells your brain you did good.
