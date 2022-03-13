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

field_type          : (STRING_T | INTARRAY_T | BYTEARRAY_T) '[' type_size=DECIMAL ']'
                    | BYTE_T
                    | INTEGER_T
                    | UNSIGNED_T
                    | FLOAT_T
                    | DOUBLE_T
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

expression          : expression_factor
                    ;

expression_factor   : expression_factor PLUS expression_factor   # ExpPlus
                    | expression_factor MINUS expression_factor  # ExpMinus
                    | term                                       # ExpTerm
                    ;

term
                    : term STAR term               # ExpMult
                    | term DIVIDE term             # ExpDiv
                    | '(' expression ')'           # ExpIn
                    | factor                       # ExpFactor
                    ;

factor              : FLOAT                        # ExpFloat
                    | DECIMAL                      # ExpDec
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
                    | stream_factor AT '(' window=DECIMAL COMMA step=DECIMAL ')' # SExpAgse
                    | stream_factor DOT agregator       # SExpAgregate
                    | stream_factor                     # SExpFactor
                    ;

stream_factor       : ID
                    | '(' stream_expression ')'
                    ;

agregator           : MIN | MAX | AVG | SUMC
                    ;

function_call       : ( 'Sqrt'
                    | 'Ceil'
                    | 'Abs'
                    | 'Floor'
                    | 'Sign'
                    | 'Chr'
                    | 'Length'
                    | 'ToNumber'
                    | 'ToTimeStamp'
                    | 'FloatCast'
                    | 'InstCast'
                    | 'Count'
                    | 'Crc'
                    | 'Sum'
                    | 'IsZero'
                    | 'IsNonZero' ) '(' expression_factor ')'
                    ;


// sync types with: src/include/rdb/desc.h

STRING_T:           'STRING'|'String';
BYTEARRAY_T:        'BYTEARRAY'|'Bytearray';
INTARRAY_T:         'INTARRAY'|'Intarray';
BYTE_T:             'BYTE'|'Byte';
UNSIGNED_T:         'UNSIGNED'|'Uint';
INTEGER_T:          'INT'|'Int'|'INTEGER';
FLOAT_T:            'FLOAT'|'Float';
DOUBLE_T:           'DOUBLE'|'Double';

SELECT:             'SELECT'|'select';
STREAM:             'STREAM'|'stream';
FROM:               'FROM'|'from';
DECLARE:            'DECLARE'|'declare';
FILE:               'FILE'|'file';

MIN:                'MIN'|'min';
MAX:                'MAX'|'max';
AVG:                'AVG'|'avg';
SUMC:               'SUMC'|'sumc';

ID:                 ([A-Za-z]) ([A-Za-z_$0-9])*;
STRING:             'N'? '\'' (~'\'' | '\'\'')* '\'';
FLOAT:              DEC_DOT_DEC;
DECIMAL:            MINUS? DEC_DIGIT+;
UDECIMAL:           DEC_DIGIT+;
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