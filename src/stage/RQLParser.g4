parser grammar RQLParser;

// https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4

options {tokenVocab=RQLLexer;}

clauses
    : select_statement
    | declare_statement
    ;

select_statement
    : SELECT columns=select_list
      STREAM ID
      FROM from=stream_expression
    ;

declare_statement
    : DECLARE full_column_name_list
      STREAM ID
      (FILE ID)?
    ;

full_column_name_list
    : column+=ID (',' column+=ID)*
    ;

select_list
    : ID
    ;

stream_expression
    : ID
    ;

