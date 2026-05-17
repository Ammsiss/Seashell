# Seashell

A simple linux shell.

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

- Make dynamic array module
- Combine LX_TOK_RDR_OUT and LX_TOK_RDR_STDOUT

### Lexer

Interesting example: ```$ A="bye" sh -c 'echo "$A"'```

## License
Seashell is licensed under the MIT License. See [LICENSE](LICENSE) for details.
