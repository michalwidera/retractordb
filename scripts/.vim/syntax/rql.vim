" Vim syntax file
" Language:	RetractorDB Query Language
" Maintaner:	Michal Widera (michal@widera.com.pl)

if exists("b:current_syntax")
  finish
endif

syn case match
syn iskeyword A-Z,a-z,48-57,_,$

" RQL statement keywords (SELECT, DECLARE, RULE, etc.)
syn keyword rqlKey          SELECT select STREAM stream FROM from DECLARE declare
syn keyword rqlKey          FILE file VOLATILE volatile RULE rule ON on
syn keyword rqlKey          BINFILE binfile TEXTFILE textfile DEVICE device
syn keyword rqlKey          WHEN when DO do DUMP dump TO to SYSTEM system
syn keyword rqlKey          DISPOSABLE disposable ONESHOT oneshot HOLD hold TIMEOUT timeout
syn keyword rqlKey          RETENTION retention PERSISTENT persistent
syn keyword rqlKey          LET let VAR var nextgroup=rqlNamedValue skipwhite
syn keyword rqlKey          SET set

" Nazwy deklaracji LET/VAR i wspolne odwolania ?NAZWA.
syn match rqlNamedValue     "\<[A-Za-z][A-Za-z_0-9]*\>" contained
syn match rqlNamedValue     "?[A-Za-z][A-Za-z_0-9]*\%([A-Za-z_$0-9]\)\@!"

" Compiler option directives
syn keyword rqlDirective    STORAGE storage ROTATION rotation SUBSTRAT substrat

" Logical operators
syn keyword rqlLogic        AND and OR or NOT not

" Storage profile values (TYPE_PROFILE)
syn keyword rqlProfile      MEMORY memory DEFAULT default DIRECT direct
syn keyword rqlProfile      POSIX posix POSIXSHD posixshd GENERIC generic

" Operatory strumieniowe i skalarne; # wewnatrz wiersza jest przeplotem.
syn match rqlOperator       "[:=<>!#&%@+$*/~^|-]\+"

" Komentarz # tylko od pierwszego niebialego znaku wiersza.
syn match  rqlComment       "^[ \t]*#.*$"
syn match  rqlComment       "//.*$"
syn region rqlBlockComment  start="/\*" end="\*/" contains=rqlBlockComment

" Strings: single-quoted only ('' is escaped quote inside string)
syn region rqlString        start=+'+  skip=+''+  end=+'+  contains=rqlStringEscape
syn match  rqlStringEscape  "''"  contained

" Numbers: integer, float, fraction (rational)
syn match rqlNumber         "\<[0-9]\+\>"
syn match rqlNumber         "\%(\<[0-9]\+\.[0-9]*\|\.[0-9]\+\)"
syn match rqlNumber         "\%(\<[0-9]\+\%(\.[0-9]*\)\=\|\.[0-9]\+\)E[+-]\=[0-9]\+\>"
syn match rqlNumber         "\<[0-9]\+\/[0-9]\+\>"

" Data types
syn keyword rqlType         BYTE Byte CHAR Char UINT Uint STRING String
syn keyword rqlType         FLOAT Float INTEGER Integer DOUBLE Double
syn keyword rqlType         RATIONAL Rational

" Aggregate functions
syn keyword rqlAggregate    MIN min MAX max AVG avg SUMC sumc

" Funkcje i skroty z src/include/rqlFunctions.hpp, bez wzgledu na wielkosc liter.
syn match rqlFunction       "\c\<\%(sqrt\|ceil\|floor\|abs\|round\|trunc\|sin\|cos\|exp\|tan\|log2\|log\|isnull\|null2zero\|iszero\|isnonzero\|length\|to_integer\|to_float\|to_double\|to_string\|int\|float\|real\|str\)\>\ze\s*("

hi def link rqlKey          Keyword
hi def link rqlDirective    PreProc
hi def link rqlLogic        Operator
hi def link rqlOperator     Operator
hi def link rqlProfile      Constant
hi def link rqlNamedValue   Identifier
hi def link rqlComment      Comment
hi def link rqlBlockComment Comment
hi def link rqlString       String
hi def link rqlStringEscape SpecialChar
hi def link rqlNumber       Number
hi def link rqlType         Type
hi def link rqlAggregate    Function
hi def link rqlFunction     Function

let b:current_syntax = "rql"
