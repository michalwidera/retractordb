# RQL grammar and parser — known pitfalls

Applies to `RQL.g4`, `RQLParser.cpp` and `expressionEvaluator.cpp` in this directory.

### COMMA ambiguity in `select_list` vs `function_call`

`select_list` uses `COMMA` to separate expressions. ANTLR4 SLL mode ignores call-stack context → `f(a, b)` inside multi-item SELECT parses as `f(a)`, `, b` treated as list separator.

**Rule:** Never use `COMMA` in function arguments in `RQL.g4`. Use `COLON` or another token absent from `select_list`.

Current pattern: `to_string(expr : N)` — `:` (COLON) = output field width.

### Adding a scalar function

Function names are **not** grammar literals. `function_call` takes a plain `ID`, and the single
list of legal names lives in `src/include/rqlFunctions.hpp`. Adding a one-argument function:

1. Add a row to `kRqlFunctions` in `src/include/rqlFunctions.hpp` — canonical name plus arity
2. Add a branch in the `CALL` chain in `src/retractor/lib/expressionEvaluator.cpp` — match the
   **lowercased** name; the parser stores the canonical spelling, the evaluator folds case
3. No grammar edit, no `ninja rqlgrammar`, no new `command_id`

`canonical` is what the parser writes into the token, regardless of how the author spelled it.
Keep it stable: plan dumps print it (`CALL(Sqrt)`), integration `pattern.txt` files and the H9
pilot plans under `test/research_gate/h9/pilot/out/` match on it, and `exitExpression` /
`exprSimplify` compare `getStr_() == "to_string"` literally.

`compiler::checkFunctionCalls()` rejects an unknown name or a bad arity through the
`Check result:` channel, so `-c` is a gate: a program that cannot run does not compile.

**Two-argument functions.** The grammar has exactly two shapes — `f(expr)` and
`f(expr : DECIMAL)`. The second belongs to `to_string` alone: `N` is the declared output field
width carried in the token as `IDXPAIR`, not a value on the stack. A future function taking two
**evaluated** arguments needs one more grammar alternative
(`fn=ID '(' expression_factor ( COLON expression_factor )* ')'` is verified to parse) plus the
arity row — never a `COMMA` separator, see the rule above.

**`min` is unavailable as a scalar function name.** `MIN`, `MAX`, `AVG` and `SUMC` are lexer
tokens ahead of `ID` (stream reducers), so they never lex as a function name. A scalar minimum
must be called something else.

### Descriptor field sizes for STRING expressions in SELECT

`exitExpression` in `RQLParser.cpp` sums output field size from `program` tokens:
- `CALL2` + `to_string` → `pair.second` (declared width)
- `CALL` + `to_string` (no width) → 32 (default)
- `PUSH_VAL` string literal → `string.length()`
- Summed: `to_string(x:16)+'_test'` → 16+5=21
