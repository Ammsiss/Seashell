# Seashell

A simple linux shell.

## Features

- Execution: `./a` OR `a`
- Builtins: `cd / exit`
- Piping: `a | b |...`
- Redirection: `./command > file`
- Job control: `./command & / jobs / fg / bg`

## TODO


### Lexer

Add tokens for: >>, <<, EOF, ;, \n, ||, &&, (, )
Handle: \

Lexer should preserve quotes either directly or as
meta data.

## THINKING!!

```bash
FOO=bar echo "$FOO()" | grep "()" > ~/file.txt &
```

The lexer takes raw command input and converts it to
simple tokens.

tokens:
    word(FOO=bar)
    word(echo)
    word("$FOO()")
    pipe()
    word(grep)
    word("()")
    rdr_out()
    word(~/file.txt)
    bg()

The parser groups related tokens into a structural command
representation. The parser sets up structure for stuff like
pipelines, subshells, command substitution, setting env
variables, redirections, semi clolons, etc.

background:
    yes
PIPELINE
  COMMAND
    assignments:
      word(FOO=bar)
    words:
      word(echo)
      word("$FOO()")
  COMMAND
    redirections:
      out -> word(~/file.txt)
    words:
      word(grep)
      word("()")

The expander analyzes the individual words in the now created
structural representation and expands them if needed. It is
concerned with $ (for expansions), basic globbing with *,
expanding env variables, ~ expansion, command substitution.

background:
  yes
PIPELINE
  COMMAND
    assignments:
      word(FOO=bar)
    words:
      word(echo)
      word(()) <- expanded $FOO
  COMMAND
    redirections:
      out -> word(/home/user/file.txt) <- expanded ~
    words:
      word(grep)
      word(())

The executor now takes the syntactically valid command structure
and spins up the job(s)

```c
/* In a background process group. The subshell execing echo has FOO=bar */
exec("echo", "()") /* | */ exec("grep", "()") /* > */ open("/home/user/file.txt")
```

Interesting example:
```bash
  $ A="bye" sh -c 'echo "$A"'
```

## License
Seashell is licensed under the MIT License. See [LICENSE](LICENSE) for details.
