grammar RQL;

@header {
    // https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4
    #include <iostream>
}

prog                : ( select_statement | declare_statement )+ EOF
                    ;

select_statement    : SELECT columns=select_list
                      STREAM id=ID
                      FROM from=stream_expression
                      {
                      std::cout << "::columns ID:" << $columns.text << std::endl;
                      std::cout << "::id ID:" << $id.text << std::endl;
                      std::cout << "::from ID:" << $from.text << std::endl;
                      }

                    ;

declare_statement   : DECLARE column_name_list
                      STREAM ID COMMA ( DECIMAL | FLOAT | RATIONAL )
                      (FILE STRING)?
                    ;

column_name_list    : column+=ID column_type (COMMA column+=ID column_type)*
                    ;

column_type         : ( ( FLOAT_T | BYTE_T | INTEGER_T | UNSIGNED_T )
                        | ( ( STRING_T | ARRAY_B_T | ARRAY_I_T ) LS_BRACKET type_size=DECIMAL RS_BRACKET )
                      )
                    ;

select_list         : column+=select_list_elem (COMMA column+=select_list_elem)*
                    ;

select_list_elem    : asterisk
                    | expression
                    ;

field_id            : column_name=ID // id
                    | column_name=ID LS_BRACKET UNDERLINE RS_BRACKET // id[_]
                    | tablename=ID DOT column_name=ID  // id.id
                    | tablename=ID LS_BRACKET column_index=DECIMAL RS_BRACKET // id[id]
                    ;
/*
expression
    :
    | function_call
    | unary_op_expression
    | expression op=('*' | '/' | '%') expression
    | expression op=('+' | '-' | '&' | '^' | '|' | '||') expression
    ;
*/
unary_op_expression
    : BIT_NOT expression
    | op=( PLUS | MINUS ) expression
    ;

asterisk            : (ID DOT)? STAR
                    ;

expression          : term ( PLUS term | MINUS term )*
                    ;

term                : factor ( STAR factor | DIVIDE factor )*
                    ;

factor              : FLOAT
                    | REAL
                    | DECIMAL
                    | unary_op_expression
                    | function_call
                    | field_id
                    | agregator
                    | LR_BRACKET expression RR_BRACKET
                    ;

stream_expression   : stream_term
                      ( GREATER DECIMAL
                      | PLUS stream_term
                      | MINUS RATIONAL
                      ) *
                    ;

stream_term         : ID
                      ( SHARP ID
                      | AND RATIONAL
                      | MOD RATIONAL
                      | AT LR_BRACKET DECIMAL COMMA (PLUS|MINUS)? DECIMAL RR_BRACKET
                      | DOT agregator
                      ) *
                    ;

agregator           : MIN | MAX | AVG | SUMC
                    ;

function_call       : function = (SQRT | CEIL | ABS | FLOOR | SIGN | CHR
                       | LENGTH | TO_NUMBER | TO_TIMESTAMP | FLOAT_CAST
                       | INT_CAST | COUNT | CRC | SUM | ISZERO | ISNONZERO )
                      LR_BRACKET expression RR_BRACKET
                    ;

FLOAT_T:            'FLOAT';
STRING_T:           'STRING';
BYTE_T:             'BYTE';
INTEGER_T:          'INT';
UNSIGNED_T:         'UNSIGNED';
ARRAY_B_T:          'ARRAY_BYTE';
ARRAY_I_T:          'ARRAY_INTEGER';

SELECT:             'SELECT';
STREAM:             'STREAM';
FROM:               'FROM';
DECLARE:            'DECLARE';
FILE:               'FILE';

MIN:                'MIN';
MAX:                'MAX';
AVG:                'AVG';
SUMC:               'SUMC';

SQRT:               'SQRT';
CEIL:               'CEIL';
ABS:                'ABS';
FLOOR:              'FLOOR';
SIGN:               'SIGN';
CHR:                'CHR';
LENGTH:             'LENGTH';
TO_NUMBER:          'TO_NUMBER';
TO_TIMESTAMP:       'TO_TIMESTAMP';
FLOAT_CAST:         'FLOATC';
INT_CAST:           'INTC';
COUNT:              'COUNT';
CRC:                'CRC';
SUM:                'SUM';
ISZERO:             'ISZERO';
ISNONZERO:          'ISNONZERO';

ID:                 ( [A-Za-z] ) ( [A-Za-z$0-9] )*;
STRING:             'N'? '\'' (~'\'' | '\'\'')* '\'';
FLOAT:              DEC_DOT_DEC;
DECIMAL:            DEC_DIGIT+;
RATIONAL:           DECIMAL DIVIDE DECIMAL;
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
AND:                '&';
MOD:                '%';
DOLLAR:             '$';
LR_BRACKET:         '(';
RR_BRACKET:         ')';
LS_BRACKET:         '[';
RS_BRACKET:         ']';
LC_BRACKET:         '{';
RC_BRACKET:         '}';
COMMA:              ',';
SEMI:               ';';
COLON:              ':';
DOUBLE_COLON:       '::';
STAR:               '*';
DIVIDE:             '/';
PLUS:               '+';
MINUS:              '-';
BIT_NOT:            '~';
BIT_OR:             '|';
BIT_XOR:            '^';

SPACE:              [ \t\r\n]+    -> skip;
COMMENT:            '/*' (COMMENT | .)*? '*/' -> channel(HIDDEN);
LINE_COMMENT:       '--' ~[\r\n]* -> channel(HIDDEN);

fragment LETTER:    [A-Z_];
fragment DEC_DOT_DEC: (DEC_DIGIT+ '.' DEC_DIGIT+ |  DEC_DIGIT+ '.' | '.' DEC_DIGIT+);
fragment HEX_DIGIT: [0-9A-F];
fragment DEC_DIGIT: [0-9];