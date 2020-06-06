" Vim syntax file
" Language:	RetractorDB Query Language
" Maintaner:	Michal Widera (michal@widera.com.pl)

if exists("b:current_syntax")
  finish
endif

syn case ignore

" basic
syn keyword rqlKey	declare file select stream from

" comment
syn match  rqlComment       "^#.*$"

" Strings
syn region rqlString        start=+"+  skip=+\\\\\|\\"+  end=+"+
syn region rqlString        start=+'+  skip=+\\\\\|\\'+  end=+'+
syn region rqlString        start=+`+  skip=+\\\\\|\\`+  end=+`+

" Numbers
syn match rqlNumber         "\<[0-9]*\>"
syn match rqlNumber         "\<[0-9]*\.[0-9]*\>"
syn match rqlNumber         "\<[0-9][0-9]*e[+-]\=[0-9]*\>"
syn match rqlNumber         "\<[0-9]*\.[0-9]*e[+-]\=[0-9]*\>"
syn match rqlNumber         "\<[0-9]*\/[0-9]*\>"
syn match rqlNumber         "\<0x[abcdef0-9]*\>"

" Types
syn keyword rqlType         byte char i8 i16 integer int rational rat

hi def link rqlNumber       Number
hi def link rqlString       String
hi def link rqlKey          Keyword
hi def link rqlComment      Comment
hi def link rqlType         Type

let b:current_syntax = "rql"

