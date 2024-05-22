grammar DESC;

desc                  : '{' command* '}'
                      ;

command               : BYTE_T name=ID     ('[' arr=DECIMAL ']')?  # ByteID                      
                      | INTEGER_T name=ID  ('[' arr=DECIMAL ']')?  # IntegerID     
                      | UNSIGNED_T name=ID ('[' arr=DECIMAL ']')?  # UnsignedID
                      | FLOAT_T name=ID    ('[' arr=DECIMAL ']')?  # FloatID
                      | DOUBLE_T name=ID   ('[' arr=DECIMAL ']')?  # DoubleID
                      | RATIONAL_T name=ID ('[' arr=DECIMAL ']')?  # RationalID
                      | REF_T '"' file=FILENAME '"'                # RefID
                      | TYPE_T type=ID                             # TypeID
                      | STRING_T name=ID '[' strsize=DECIMAL ']'   # StringID
                      | SEGMENT_T size=DECIMAL                     # SegmentID
                      | CAPACITY_T size=DECIMAL                    # CapacityID
                      ;

// sync types with: src/include/rdb/fldType.h

BYTE_T:             'BYTE';
STRING_T:           'STRING';
UNSIGNED_T:         'UINT';
INTEGER_T:          'INTEGER';
FLOAT_T:            'FLOAT';
DOUBLE_T:           'DOUBLE';
RATIONAL_T:         'RATIONAL';
INTPAIR_T:          'INTPAIR';
IDXPAIR_T:          'IDXPAIR';

fragment DEC_DIGIT: [0-9];

TYPE_T:             'TYPE'; 
REF_T:              'REF';

SEGMENT_T:          'SEGMENT';
CAPACITY_T:         'CAPACITY';

DOT:                '.';
ID:                 ([A-Za-z]) ([A-Za-z_$0-9])*;
STRING:             '\'' (~'\'' | '\'\'')* '\'';
DECIMAL:            DEC_DIGIT+;
REF_TYPE_ARG:       'TEXTSOURCE' | 'DEVICE' | 'GENERIC' | 'DEFAULT';
FILENAME:           ([A-Za-z_$0-9]|DOT)+ ;

SPACE:              [ \t\r\n]+    -> skip;
