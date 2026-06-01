# Seashell

A simple linux shell. Collect, compute, describe, perform.

## Capabilities

- Execution: `./a` OR `a`
- Builtins: `cd / exit`
- Job control: `./cmd & / jobs / fg / bg`

- Piping: `a | b |...`
- Redirection: `./cmd > f1 2> f2 >> f3`
- Multiple jobs: `ls ; ls ..`

- Variables: `FOO=dir ; (ls ~/$FOO)`
- Globbing: `echo *.txt`
- Command substitution: `cat $(ls)`

## TODO

- Lexer should handle adding quote meta data, and stripping quotes.

## License
Seashell is licensed under the MIT License. See [LICENSE](LICENSE) for details.
