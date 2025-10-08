grammar RQL;

prog                : ( select_statement
                      | declare_statement
                      | storage_statement
                      | substrat_statement
                      | compiler_option
                      | rule_statement
                      )+ EOF
                    ;

compiler_option     : COPTION compiler_options=STRING
                    # Coption
                    ;

storage_statement   : STORAGE folder_name=STRING
                    # Storage
                    ;

substrat_statement  : SUBSTRAT substrat_type=STRING_SUBSTRAT
                    # Substrat
                    ;

select_statement    : SELECT select_list
                      STREAM stream_name=ID
                      FROM stream_expression
                      (FILE name=STRING)?
                      (retention_from)?
                    # Select
                    ;

declare_statement   : DECLARE field_declaration (COMMA field_declaration)*
                      STREAM stream_name=ID COMMA rational_se
                      FILE file_name=STRING
                    # Declare
                    ;

rule_statement      : RULE name=ID
                      ON stream_name=ID
                      WHEN logic
                      DO ( dumppart | systempart )
                    # Rulez
                    ;

dumppart            : DUMP '-'? step_back=DECIMAL TO '-'? step_forward=DECIMAL (RETENTION rule_retnetion=DECIMAL)?
                    ;

systempart          : SYSTEM syscmd=STRING
                    ;

rational_se         : fraction_rule # RationalAsFraction_proforma
                    | FLOAT         # RationalAsFloat
                    | DECIMAL       # RationalAsDecimal
                    ;

retention_from      : RETENTION capacity=DECIMAL (segments=DECIMAL)?
                    # Retention
                    ;

fraction_rule       : DECIMAL DIVIDE DECIMAL
                    # Fraction
                    ;


field_declaration   : ID field_type ('[' type_size=DECIMAL ']')?
                    # SingleDeclaration
                    ;

field_type          : BYTE_T     # typeByte
                    | INTEGER_T  # typeInt
                    | UNSIGNED_T # typeUnsigned
                    | FLOAT_T    # typeFloat
                    | DOUBLE_T   # typeDouble
                    | STRING_T   # typeString
                    ;

select_list         : asterisk                       # SelectListFullscan
                    | expression (COMMA expression)* # SelectList
                    ;

field_id            : column_name=ID                             # FieldID            // id    - ID3
                    | tablename=ID '[' UNDERLINE ']'             # FieldIDUnderline   // id[_] - IDX
                    | tablename=ID DOT column_name=ID            # FieldIDColumnName  // id.id - ID1
                    | tablename=ID '[' column_index=DECIMAL ']'  # FieldIDTable       // id[x] - ID2
                    ;

unary_op_expression : BIT_NOT expression
                    | op=(PLUS | MINUS) expression
                    ;

asterisk            : (ID DOT)? STAR
                    ;

expression          : expression_factor
                    ;

logic               : expression_logic
                    # LogicExpression
                    ;

expression_logic    : expression_logic AND_C expression_logic    # ExpAnd
                    | expression_logic OR_C expression_logic     # ExpOr
                    | term_logic                                 # ExpTermLogic
                    ;

term_logic          : term_logic IS_EQ term_logic                # ExpEq
                    | term_logic IS_NQ term_logic                # ExpNq
                    | term_logic IS_GR term_logic                # ExpGr
                    | term_logic IS_LS term_logic                # ExpLs
                    | term_logic IS_GE term_logic                # ExpGe
                    | term_logic IS_LE term_logic                # ExpLe 
                    | expression_factor                          # ExpFactor
                    ;

expression_factor   : expression_factor PLUS expression_factor   # ExpPlus
                    | expression_factor MINUS expression_factor  # ExpMinus
                    | term                                       # ExpTerm
                    ;

term                : term STAR term               # ExpMult
                    | term DIVIDE term             # ExpDiv
                    | fraction_rule                # ExpRational
                    | '(' expression_factor ')'    # ExpIn
                    | '-'? FLOAT                   # ExpFloat
                    | '-'? DECIMAL                 # ExpDec
                    | STRING                       # ExpString
                    | unary_op_expression          # ExpUnary
                    | field_id                     # ExpField
                    | agregator                    # ExpAgg
                    | function_call                # ExpFnCall
                    | NOT_C term                   # ExpNot
                    ;

stream_expression   : stream_term '>' DECIMAL       # SExpTimeMove
                    | stream_term MINUS rational_se # SExpMinus
                    | stream_term PLUS stream_term  # SExpPlus
                    | stream_term                   # SExpTerm
                    ;

stream_term         : stream_factor SHARP stream_factor # SExpHash
                    | stream_factor AND rational_se     # SExpAnd
                    | stream_factor MOD rational_se     # SExpMod
                    | stream_factor AT '(' step=DECIMAL COMMA '-'? window=DECIMAL ')' # SExpAgse
                    | stream_factor DOT agregator       # SExpAgregate_proforma
                    | stream_factor                     # SExpFactor
                    ;

stream_factor       : ID
                    | '(' stream_expression ')'
                    ;

agregator           : MIN   # StreamMin
                    | MAX   # StreamMax
                    | AVG   # StreamAvg
                    | SUMC  # StreamSum
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
                    | 'IntCast'
                    | 'Count'
                    | 'Crc'
                    | 'Sum'
                    | 'IsZero'
                    | 'IsNonZero' ) '(' expression_factor ( COMMA expression_factor )* ')'
                    ;

// sync types with: src/include/rdb/fldType.hpp

BYTE_T:             'BYTE'|'Byte'|'CHAR'|'Char';
STRING_T:           'STRING'|'String';
UNSIGNED_T:         'UINT'|'Uint';
INTEGER_T:          'INTEGER'|'Integer';
FLOAT_T:            'FLOAT'|'Float';
DOUBLE_T:           'DOUBLE'|'Double';

SELECT:             'SELECT'|'select';
STREAM:             'STREAM'|'stream';
FROM:               'FROM'|'from';
DECLARE:            'DECLARE'|'declare';
RETENTION:          'RETENTION'|'retention';
FILE:               'FILE'|'file';
STORAGE:            'STORAGE'|'storage';
COPTION:            'COPTION'|'coption';
SUBSTRAT:           'SUBSTRAT'|'substrat';
RULE:               'RULE'|'rule';
ON:                 'ON'|'on';
WHEN:               'WHEN'|'when';
DUMP:               'DUMP'|'dump';
SYSTEM:             'SYSTEM'|'system';
DO:                 'DO'|'do';
TO:                 'TO'|'to';
AND_C:              'AND'|'and';
OR_C:               'OR'|'or';
NOT_C:              'NOT'|'not';

MIN:                'MIN'|'min';
MAX:                'MAX'|'max';
AVG:                'AVG'|'avg';
SUMC:               'SUMC'|'sumc';

STRING_SUBSTRAT:    '\'' ('MEMORY'|'memory'|'DEFAULT'|'default'|'POSIX'|'posix'|'GENERIC'|'generic'|'DEVICE'|'device'|'TEXTSOURCE'|'textsource') '\'';

ID:                 ([A-Za-z]) ([A-Za-z_$0-9])*;
STRING:             '\'' (~'\'' | '\'\'')* '\'';
FLOAT:              DEC_DOT_DEC;
DECIMAL:            DEC_DIGIT+;
REAL:               (DECIMAL | DEC_DOT_DEC) ('E' [+-]? DEC_DIGIT+);

IS_EQ:              '=';
IS_NQ:              '!=';
IS_GR:              '>';
IS_LS:              '<';
IS_GE:              '>=';
IS_LE:              '<=';
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
// note: there must be space after the hash - otherwise a#b is recognized as a #comment.
LINE_COMMENT1:      '# ' ~[\r\n]* -> channel(HIDDEN);
LINE_COMMENT2:      '//' ~[\r\n]* -> channel(HIDDEN);

fragment LETTER:    [A-Z_];
fragment DEC_DOT_DEC: (DEC_DIGIT+ '.' DEC_DIGIT+ |  DEC_DIGIT+ '.' | '.' DEC_DIGIT+);
fragment HEX_DIGIT: [0-9A-F];
fragment DEC_DIGIT: [0-9];