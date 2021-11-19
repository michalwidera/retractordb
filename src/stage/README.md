RetractorDB - Staging Area
-----------------------------------------------------------------------

use:
```
conan install .. -s build_type=Debug
conan build ..
xstage
make install
xstage

sudo apt install antlr4
antlr4 -o Parser -Dlanguage=Cpp Expr.g4
...
```

Note: Assertions works only here!
