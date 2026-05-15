# Seashell

A simple linux shell.

## Features

- Execution: `./a` OR `a`
- Builtins: `cd / exit`
- Piping: `a | b |...`
- Redirection: `./command > file`
- Job control: `./command & / jobs / fg / bg`

## TODO

- Remove strlen() call in lx_tokanize() loop
- Add null checks for all lx_push_token() calls
- Add free() calls before returning NULL in lx_tokanize()
- Maybe always initialze all fields in lx_token (good for debugging)
- Don't return NULL on cmd == "" in lx_tokanize()
- Add custom error nums for lx_tokanize()
- change int -> size_t for all arrays
- Change the enum name for '"' and ' '; they aren't really tokens
- Make a helper function to add tokens

## License
Seashell is licensed under the MIT License. See [LICENSE](LICENSE) for details.
