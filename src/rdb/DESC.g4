grammar DESC;

desc                  : '{' command* '}'
                      ;

command               : BYTE_T name=ID 
                      | STRING_T name=ID
                      | UNSIGNED_T name=ID
                      | FLOAT_T name=ID
                      | DOUBLE_T name=ID
                      | REF_T name=STRING
                      | TYPE_T REF_TYPE_ARG
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

TYPE_T:             'TYPE'; 
REF_T:              'REF';

ID:                 ([A-Za-z]) ([A-Za-z_$0-9])*;
STRING:             '\'' (~'\'' | '\'\'')* '\'';
REF_TYPE_ARG:       'TEXTSOURCE' | 'DEVICE' | 'GENERIC' | 'DEFAULT';
