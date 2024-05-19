grammar DESC;

desc                  : '{' command* '}'
                      ;

command               : BYTE_T name=ID                         # Byte
                      | STRING_T name=ID '[' DECIMAL ']'       # String
                      | UNSIGNED_T name=ID                     # Unsigned
                      | FLOAT_T name=ID                        # Float
                      | DOUBLE_T name=ID                       # Double
                      | REF_T name=STRING '"' file=STRING '"'  # Ref
                      | TYPE_T type=REF_TYPE_ARG               # Type
                      ;

// sync types with: src/include/rdb/fldType.h

BYTE_T:             'BYTE';
STRING_T:           'STRING';
UNSIGNED_T:         'UINT';
INTEGER_T:          'INTEGER';
FLOAT_T:            'FLOAT';
DOUBLE_T:           'DOUBLE';
RATIONAL_T:         'RATIONAl';
INTPAIR_T:          'INTPAIR';
IDXPAIR_T:          'IDXPAIR';

fragment DEC_DIGIT: [0-9];

TYPE_T:             'TYPE'; 
REF_T:              'REF';

ID:                 ([A-Za-z]) ([A-Za-z_$0-9])*;
STRING:             '\'' (~'\'' | '\'\'')* '\'';
DECIMAL:            DEC_DIGIT+;
REF_TYPE_ARG:       'TEXTSOURCE' | 'DEVICE' | 'GENERIC' | 'DEFAULT';
