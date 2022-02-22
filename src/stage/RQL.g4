grammar RQL;

prog                : ( select_statement
                      | declare_statement
                      )+ EOF
                    ;

select_statement    : SELECT select_list
                      STREAM ID
                      FROM stream_expression
                    # Select
                    ;

rational            : fraction # RationalAsFraction
                    | FLOAT    # RationalAsFloat
                    | DECIMAL  # RationalAsDecimal
                    ;

fraction            : DECIMAL DIVIDE DECIMAL
                    ;

declare_statement   : DECLARE declare_list
                      STREAM stream_name=ID COMMA rational
                      FILE file_name=STRING
                    # Declare
                    ;

declare_list        : field_declaration (COMMA field_declaration)*
                    # DeclarationList
                    ;

field_declaration   : ID field_type
                    # SingleDeclaration
                    ;

field_type          : ( ( (STRING_T | INTEGER_T | BYTE_T ) '[' type_size=DECIMAL ']')
                    | (FLOAT_T | BYTE_T | INTEGER_T | UNSIGNED_T)
                    )
                    ;

select_list         : asterisk                       # SelectListFullscan
                    | expression (COMMA expression)* # SelectList
                    ;

field_id            : column_name=ID                            # FieldID            // id    - ID3
                    | tablename=ID '[' UNDERLINE ']'            # FieldIDUnderline   // id[_] - IDX
                    | tablename=ID DOT column_name=ID           # FieldIDColumnname  // id.id - ID1
                    | tablename=ID '[' column_index=DECIMAL ']' # FieldIDTable       // id[x] - ID2
                    ;

unary_op_expression : BIT_NOT expression
                    | op=(PLUS | MINUS) expression
                    ;

asterisk            : (ID DOT)? STAR
                    ;

expression
                    : expression PLUS expression   # ExpPlus
                    | expression MINUS expression  # ExpMinus
                    | term                         # ExpTerm
                    ;

term
                    : term STAR term               # ExpMult
                    | term DIVIDE term             # ExpDiv
                    | '(' expression ')'           # ExpIn
                    | factor                       # ExpFactor
                    ;

factor              : w=FLOAT                      # ExpFloat
                    | w=DECIMAL                    # ExpDec
                    | unary_op_expression          # ExpUnary
                    | function_call                # ExpFnCall
                    | field_id                     # ExpField
                    | agregator                    # ExpAgg
                    ;

stream_expression   : stream_term GREATER DECIMAL  # SExpTimeMove
                    | stream_term PLUS stream_term # SExpPlus
                    | stream_term MINUS rational   # SExpMinus
                    | stream_term                  # SExpTerm
                    ;

stream_term         : stream_factor SHARP stream_factor # SExpHash
                    | stream_factor AND rational        # SExpAnd
                    | stream_factor MOD rational        # SExpMod
                    | stream_factor AT '(' DECIMAL COMMA (PLUS | MINUS)? DECIMAL ')' # SExpAgse
                    | stream_factor DOT agregator       # SExpAgregate
                    | stream_factor                     # SExpFactor
                    ;

stream_factor       : ID
                    | '(' stream_expression ')'
                    ;

agregator           : MIN | MAX | AVG | SUMC
                    ;

function_call       : function = (SQRT | CEIL | ABS | FLOOR | SIGN | CHR
                       | LENGTH | TO_NUMBER | TO_TIMESTAMP | FLOAT_CAST
                       | INT_CAST | COUNT | CRC | SUM | ISZERO | ISNONZERO)
                      '(' expression ')'
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
LINE_COMMENT:       '# ' ~[\r\n]* -> channel(HIDDEN);

fragment LETTER:    [A-Z_];
fragment DEC_DOT_DEC: (DEC_DIGIT+ '.' DEC_DIGIT+ |  DEC_DIGIT+ '.' | '.' DEC_DIGIT+);
fragment HEX_DIGIT: [0-9A-F];
fragment DEC_DIGIT: [0-9];