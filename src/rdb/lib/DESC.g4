grammar DESC;

desc                  : '{' command* '}'
                      ;

command               : BYTE_T name=ID     ('[' arr=DECIMAL ']')?  # ByteID                      
                      | INTEGER_T name=ID  ('[' arr=DECIMAL ']')?  # IntegerID     
                      | UNSIGNED_T name=ID ('[' arr=DECIMAL ']')?  # UnsignedID
                      | FLOAT_T name=ID    ('[' arr=DECIMAL ']')?  # FloatID
                      | DOUBLE_T name=ID   ('[' arr=DECIMAL ']')?  # DoubleID
                      | REF_T name=STRING '"' file=STRING '"'      # RefID
                      | TYPE_T type=REF_TYPE_ARG                   # TypeID
                      | STRING_T name=ID '[' DECIMAL ']'           # StringID
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
