grammar RQL;

@header {
// @header from RQL.g4
#include <iostream>
// End of @header
}

@members {
// @members from RQL.g4
// End of @members
}

prog                : (select_statement | declare_statement)+ EOF
                    ;

select_statement    : SELECT columns=select_list
                      STREAM id=ID
                      FROM from=stream_expression {
                      std::cout << "::columns ID:" << $columns.text << std::endl;
                      std::cout << "::id ID:" << $id.text << std::endl;
                      std::cout << "::from ID:" << $from.text << std::endl;
                      }
                    ;

rational            : FLOAT | REAL | DECIMAL | RATIONAL;

declare_statement   : DECLARE column_name_list
                      STREAM stream_name=ID COMMA rational
                      (FILE STRING)?
                    ;

column_name_list    : column+=ID column_type (COMMA column+=ID column_type)*
                    ;

column_type         : ( ( (STRING_T | INTEGER_T | BYTE_T ) LS_BRACKET type_size=DECIMAL RS_BRACKET)
                        | (FLOAT_T | BYTE_T | INTEGER_T | UNSIGNED_T)
                      )
                    ;

select_list         : column+=select_list_elem (COMMA column+=select_list_elem)*
                    ;

select_list_elem    : asterisk
                    | expression
                    ;

field_id            : column_name=ID // id
                    | tablename=ID LS_BRACKET UNDERLINE RS_BRACKET // id[_]
                    | tablename=ID DOT column_name=ID  // id.id
                    | tablename=ID LS_BRACKET column_index=DECIMAL RS_BRACKET // id[x]
                    ;

unary_op_expression : BIT_NOT expression
                    | op=(PLUS | MINUS) expression
                    ;

asterisk            : (ID DOT)? STAR {std::cout << "# Scan All" << std::endl;}
                    ;

expression returns [std::string e]
                    : expression PLUS expression {std::cout << "# Op +" << std::endl;}
                    | expression MINUS expression {std::cout << "# Op +" << std::endl;}
                    | term
                    ;

term returns [std::string e]
                    : term STAR term {std::cout << "# Op *" << std::endl;}
                    | term DIVIDE term {std::cout << "# Op /" << std::endl;}
                    | t=factor {$e = $t.text; std::cout << "# term:" << $t.text << std::endl;}
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
                      ( timemove=GREATER DECIMAL
                      | stream_add=PLUS stream_term
                      | stream_sub=MINUS rational
                      ) *
                    ;

stream_term         : stream_factor
                      ( hash=SHARP ID
                      | dehash_div=AND rational
                      | dehash_mod=MOD rational
                      | agse=AT LR_BRACKET DECIMAL COMMA (PLUS | MINUS)? DECIMAL RR_BRACKET
                      | DOT agregator
                      ) *
                    ;

stream_factor       : ID
                    | LR_BRACKET stream_expression RR_BRACKET
                    ;

agregator           : MIN | MAX | AVG | SUMC
                    ;

function_call       : function = (SQRT | CEIL | ABS | FLOOR | SIGN | CHR
                       | LENGTH | TO_NUMBER | TO_TIMESTAMP | FLOAT_CAST
                       | INT_CAST | COUNT | CRC | SUM | ISZERO | ISNONZERO)
                      LR_BRACKET expression RR_BRACKET
                    ;

// src/include/rdb/desc.h
//        String,
//        Bytearray,
//        Uint,
//        Byte,
//        Int,
//        Float,
//        Double
STRING_T:           'STRING';
INTEGER_T:          'INT';
BYTE_T:             'BYTE';
UNSIGNED_T:         'UNSIGNED';
FLOAT_T:            'FLOAT';
DOUBLE_T:           'DOUBLE';

SELECT:             'SELECT';
STREAM:             'STREAM';
FROM:               'FROM';
DECLARE:            'DECLARE';
FILE:               'FILE';

MIN:                'MIN';
MAX:                'MAX';
AVG:                'AVG';
SUMC:               'SUMC';

SQRT:               'Sqrt';
CEIL:               'Ceil';
ABS:                'Abs';
FLOOR:              'Floor';
SIGN:               'Sign';
CHR:                'Chr';
LENGTH:             'Length';
TO_NUMBER:          'ToNumber';
TO_TIMESTAMP:       'ToTimeStamp';
FLOAT_CAST:         'FloatCast';
INT_CAST:           'InstCast';
COUNT:              'Count';
CRC:                'Crc';
SUM:                'Sum';
ISZERO:             'IsZero';
ISNONZERO:          'IsNonZero';

ID:                 ([A-Za-z]) ([A-Za-z_$0-9])*;
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