## Lexer

### API

`lx_tokenize(const char *cmd, da_tok *tokens)`
- initializes the user provided `da_tok` based on the provided `cmd` string.

`lx_free(da_tok *tokens)`
- frees a `da_tok` structure previously initialized by `lx_tokenize()`

`lx_free_tok(lx_tok *tok)`
- frees an `lx_tok` structure previously initialized by `lx_tokenize()`

### Guarantees

Whitespace delimits tokens. Operators are self delimiting and can be adjacent
to words.

All text inside double quotes with the exception of the backslash are interpreted
literally.

All text inside single quotes is interpreted literally.

Adjacent quoted word parts are combined into a single word token. For example
`a"b"` would be represented as WORD(PART(a, none), PART(b, double)).

Quote delimitters are not included in the token text.

Backslashes cause the suceeding character to be interpreted literally. The
backslash itself is included in the token text.

The lexer can only fail logically if `cmd` has an unterminated single or double
quote or if there is an unescaped backslash with no succeeding character.

Unquoted whitespace separates tokens and is treated equivalently as token
boundaries. For example there is no functional difference between `cmd =
"a\t\n\r b"` and `cmd = "a b"`.

On success the `tokens` array will contain at least 1 token.

### Ownership

The caller must not initialize `tokens` before passing it to
`lx_tokenize()`

The caller must call `lx_free()` to free  the `da_tok` structure.

### Error behavior

On failure, any partial allocations are freed, and -1 is returned. You must not
call `lx_free()` on `tokens` after a failed call.

**Possible Errors**
- `cmd` is NULL
- `cmd` is an empty string
- `cmd` contains an unterminated single or double quote
- `cmd` contains an unescaped backslash with no succeeding character
- `tokens` is NULL
- A memory allocation failure
