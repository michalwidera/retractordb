grammar RQL;

// https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4

clauses             : select_statement
                    | declare_statement
                    ;

select_statement    : SELECT columns=select_list
                      STREAM ID
                      FROM from=stream_expression
                    ;

declare_statement   : DECLARE column_name_list
                      STREAM ID
                      (FILE ID)?
                    ;

column_name_list    : column+=ID (',' column+=ID)*
                    ;

select_list         : ID
                    ;

stream_expression   : ID
                    ;

SELECT:             'SELECT';
STREAM:             'STREAM';
FROM:               'FROM';
DECLARE:            'DECLARE';
FILE:               'FILE';

ID:                 ( [A-Z_#] ) ( [A-Z_#$@0-9] )*;
STRING:             'N'? '\'' (~'\'' | '\'\'')* '\'';
FLOAT:              DEC_DOT_DEC;
DECIMAL:            DEC_DIGIT+;
REAL:               (DECIMAL | DEC_DOT_DEC) ('E' [+-]? DEC_DIGIT+);

EQUAL:              '=';
GREATER:            '>';
LESS:               '<';
EXCLAMATION:        '!';
DOUBLE_BAR:         '||';
DOT:                '.';
UNDERLINE:          '_';
AT:                 '@';
SHARP:              '#';
DOLLAR:             '$';
LR_BRACKET:         '(';
RR_BRACKET:         ')';
COMMA:              ',';
SEMI:               ';';
COLON:              ':';
DOUBLE_COLON:       '::';
STAR:               '*';
DIVIDE:             '/';
MODULE:             '%';
PLUS:               '+';
MINUS:              '-';
BIT_NOT:            '~';
BIT_OR:             '|';
BIT_AND:            '&';
BIT_XOR:            '^';

SPACE:              [ \t\r\n]+    -> skip;
COMMENT:            '/*' (COMMENT | .)*? '*/' -> channel(HIDDEN);
LINE_COMMENT:       '--' ~[\r\n]* -> channel(HIDDEN);

fragment LETTER:    [A-Z_];
fragment DEC_DOT_DEC: (DEC_DIGIT+ '.' DEC_DIGIT+ |  DEC_DIGIT+ '.' | '.' DEC_DIGIT+);
fragment HEX_DIGIT: [0-9A-F];
fragment DEC_DIGIT: [0-9];