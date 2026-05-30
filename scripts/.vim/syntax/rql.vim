" Vim syntax file
" Language:	RetractorDB Query Language
" Maintaner:	Michal Widera (michal@widera.com.pl)

if exists("b:current_syntax")
  finish
endif

" RQL statement keywords (SELECT, DECLARE, RULE, etc.)
syn keyword rqlKey          SELECT select STREAM stream FROM from DECLARE declare
syn keyword rqlKey          FILE file VOLATILE volatile RULE rule ON on
syn keyword rqlKey          WHEN when DO do DUMP dump TO to SYSTEM system
syn keyword rqlKey          DISPOSABLE disposable ONESHOT oneshot HOLD hold
syn keyword rqlKey          RETENTION retention

" Compiler option directives
syn keyword rqlDirective    STORAGE storage ROTATION rotation SUBSTRAT substrat

" Logical operators
syn keyword rqlLogic        AND and OR or NOT not

" Storage profile values (TYPE_PROFILE)
syn keyword rqlProfile      MEMORY memory DEFAULT default DIRECT direct
syn keyword rqlProfile      POSIX posix POSIXSHD posixshd GENERIC generic
syn keyword rqlProfile      DEVICE device TEXTSOURCE textsource

" Comments: '# ' (space required), // and /* */
syn match  rqlComment       "# .*$"
syn match  rqlComment       "//.*$"
syn region rqlComment       start="/\*" end="\*/" contains=rqlComment

" Strings: single-quoted only ('' is escaped quote inside string)
syn region rqlString        start=+'+  end=+'+  contains=rqlStringEscape
syn match  rqlStringEscape  "''"  contained

" Numbers: integer, float, fraction (rational)
syn match rqlNumber         "\<[0-9]\+\>"
syn match rqlNumber         "\<[0-9]\+\.[0-9]\+\>"
syn match rqlNumber         "\<[0-9]\+[eE][+-]\=[0-9]\+\>"
syn match rqlNumber         "\<[0-9]\+\.[0-9]\+[eE][+-]\=[0-9]\+\>"
syn match rqlNumber         "\<[0-9]\+\/[0-9]\+\>"

" Data types
syn keyword rqlType         BYTE Byte CHAR Char UINT Uint STRING String
syn keyword rqlType         FLOAT Float INTEGER Integer DOUBLE Double

" Aggregate functions
syn keyword rqlAggregate    MIN min MAX max AVG avg SUMC sumc

" Built-in functions
syn keyword rqlFunction     Sqrt Ceil Abs Floor Sign Chr Length
syn keyword rqlFunction     ToNumber ToTimeStamp FloatCast IntCast
syn keyword rqlFunction     Count Crc Sum IsZero IsNonZero isnull
syn keyword rqlFunction     to_integer to_float to_double to_string

hi def link rqlKey          Keyword
hi def link rqlDirective    PreProc
hi def link rqlLogic        Operator
hi def link rqlProfile      Constant
hi def link rqlComment      Comment
hi def link rqlString       String
hi def link rqlStringEscape SpecialChar
hi def link rqlNumber       Number
hi def link rqlType         Type
hi def link rqlAggregate    Function
hi def link rqlFunction     Function

let b:current_syntax = "rql"
