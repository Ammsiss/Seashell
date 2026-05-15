# Seashell

A simple linux shell.

## Features

- Execution: `./a` OR `a`
- Builtins: `cd / exit`
- Piping: `a | b |...`
- Redirection: `./command > file`
- Job control: `./command & / jobs / fg / bg`

## TODO

- Rename stest_... to stress_...
- Add a header file for test files for easier navigation
- Don't return NULL on cmd == "" in lx_tokanize()
- Add custom error nums for lx_tokanize()

### Completed

- Remove strlen() call in lx_tokanize() loop
- change int -> size_t for all arrays

## License
Seashell is licensed under the MIT License. See [LICENSE](LICENSE) for details.
