grammar RQL;

prog                : ( select_statement
                      | declare_statement
                      | compiler_option
                      | rule_statement
                      )+ EOF
                    ;

compiler_option     : directive=( ROTATION | STORAGE | SUBSTRAT ) value=( STRING_PROFILE | STRING ) 
                    # Coption
                    ;

select_statement    : SELECT select_list
                      STREAM stream_name=ID
                      FROM stream_expression
                      (FILE file_name=STRING)?
                      (retention_from)?
                      (VOLATILE)?
                      (STORAGE type_name=TYPE_PROFILE)?
                    # Select
                    ;

declare_statement   : DECLARE field_declaration (COMMA field_declaration)*
                      STREAM stream_name=ID COMMA rational_se
                      FILE file_name=STRING
                      (DISPOSABLE)?
                      (ONESHOT)?
                      (HOLD)?
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

// Drabina priorytetow wyrazenia strumieniowego, od NAJMOCNIEJ wiazacego:
//
//   1. prymityw          ID | '(' e ')' | MIN/MAX/AVG/SUMC '(' e ')'
//   2. postfiks unarny   e@(k,L)  e&r  e%r  e>N  e-r  e.agg     lewostronny, lancuchowalny
//   3. przeplot          a#b                                    lewostronny
//   4. konkatenacja      a+b                                    lewostronny
//
// ANTLR4 nadaje WYZSZY priorytet alternatywom stojacym WCZESNIEJ w regule lewostronnie
// rekurencyjnej, wiec kolejnosc alternatyw ponizej JEST ta drabina. Przestawienie ich
// zmienia znaczenie jezyka, nie tylko zapis.
//
// Poziom 2 skupia SZESC operatorow o jednej postaci — operator plus literal, jeden strumien
// na wejsciu. Do 2026-08-29 byly rozrzucone na dwa pietra: `@`, `&`, `%` i `.agg` wiazaly
// mocniej niz `#`, a `>` i `-` slabiej. Bylo to niespojne szczegolnie dla `&` i `%`, ktore sa
// ODWROTNOSCIAMI `#` — staly pod operatorem, ktory odwracaja, podczas gdy `>` stal nad nim.
// Zadny z nich nie dawal sie tez lancuchowac, bo wszystkie zadaly `stream_factor`.
//
// Jedyna zmiana ZNACZENIA wzgledem tamtego stanu dotyczy `X#Y>N` i `X#Y-r`: znaczyly
// `(X#Y)>N`, znacza `X#(Y>N)`. Oba plany sa rozne (rozny ogon), dawne grupowanie zapisuje
// sie nawiasem, a roznice pilnuje test xparser.shift_binds_tighter_than_hash.
//
// Granica 3|4 wynika z typowania: `#` zada zgodnych schematow, `+` schemat poszerza, wiec
// `a#b+c` ma tylko jeden dobrze otypowany odczyt — `(a#b)+c`.
//
// Lacznosc operatorow binarnych musi zostac LEWOSTRONNA: compiler sklada nazwe substratu
// lewostronnie (`OP _arg2 _arg1`), a ta nazwa jest nazwa pliku na dysku.
stream_expression   : stream_expression AT '(' step=DECIMAL COMMA '-'? window=DECIMAL ')' # SExpAgse
                    | stream_expression AND rational_se         # SExpAnd
                    | stream_expression MOD rational_se         # SExpMod
                    | stream_expression '>' DECIMAL             # SExpTimeMove
                    | stream_expression MINUS rational_se       # SExpMinus
                    | stream_expression DOT agregator           # SExpAgregate_proforma
                    | stream_expression SHARP stream_expression # SExpHash
                    | stream_expression PLUS stream_expression  # SExpPlus
                    | stream_factor                             # SExpFactor
                    ;

// Wywolanie reduktora jest PRYMITYWEM, a nie pietrem operatorowym: jego argument domykaja
// wlasne nawiasy, tak samo jak `( e )`. Do 2026-08-29 stalo alternatywa `stream_term`, czyli
// o pietro za wysoko, wiec przechodzilo wylacznie na szczycie termu — `a#MAX(b)`,
// `MIN(a)@(1,4)` i `MIN(a)&2` byly bledami skladni, a `MIN(a)#(MAX(b))` wymagalo nawiasu
// dokladanego wylacznie po to, zeby zejsc na poziom `stream_factor`.
stream_factor       : ID
                    | '(' stream_expression ')'
                    | stream_fn_call
                    ;

// Notacja przyrostkowa `strumien.avg`. WYGASZANA na rzecz stream_fn_call.
// RQLParser::exitSExpAgregate_proforma ostrzega o kazdym uzyciu.
agregator           : MIN   # StreamMin
                    | MAX   # StreamMax
                    | AVG   # StreamAvg
                    | SUMC  # StreamSum
                    ;

// Postac funkcyjna reduktora nad CALYM wyrazeniem strumieniowym: SUMC(sq@(125,1000)).
//
// MIN, MAX, AVG i SUMC sa tokenami leksera zdefiniowanymi PRZED ID, wiec te nazwy sa
// zastrzezone: zaden strumien nie moze sie tak nazywac. `DECLARE ... STREAM min` konczy
// sie bledem skladni "mismatched input 'min' expecting ID", bo nazwa deklarowanego
// strumienia jest w regule declare_statement zwyklym ID. Zastrzezenie obejmuje pisownie
// wymienione przy tokenach, czyli
// 'MIN'|'min'; `Min` pozostaje zwykla nazwa, tak samo jak `Select` nie jest slowem
// kluczowym. Pilnuje tego test xparser.aggregate_keywords_are_reserved_stream_names.
stream_fn_call      : ( MIN
                    | MAX
                    | AVG
                    | SUMC
                    ) '(' stream_expression ')'
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
                    | 'IsNonZero'
                    | 'isnull'
                    | TO_INTEGER_FN
                    | TO_FLOAT_FN
                    | TO_DOUBLE_FN ) '(' expression_factor ( COMMA expression_factor )* ')'
                    | TO_STRING_FN '(' expression_factor ( COLON DECIMAL )? ')'
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
ROTATION:           'ROTATION'|'rotation';
SUBSTRAT:           'SUBSTRAT'|'substrat';
RULE:               'RULE'|'rule';
DISPOSABLE:         'DISPOSABLE'|'disposable';
ONESHOT:            'ONESHOT'|'oneshot';
HOLD:               'HOLD'|'hold';
VOLATILE:           'VOLATILE'|'volatile';
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

TYPE_PROFILE:       'MEMORY'|'memory'|'DEFAULT'|'default'|'DIRECT'|'direct'|'POSIX'|'posix'|'POSIXSHD'|'posixshd'|'GENERIC'|'generic'|'DEVICE'|'device'|'TEXTSOURCE'|'textsource';
STRING_PROFILE:    '\'' TYPE_PROFILE '\'';

TO_INTEGER_FN:      'to_integer';
TO_FLOAT_FN:        'to_float';
TO_DOUBLE_FN:       'to_double';
TO_STRING_FN:       'to_string';

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
// Komentarz `#` NIE jest tokenem leksera: wiersz, ktorego pierwszym niebialym znakiem jest
// `#`, odrzuca readLogicalLines() jeszcze przed parserem — tak samo w produkcji
// (launcher.cpp) jak i w testach (parserRQLFile_4Test). Dzieki temu `#` W LEKSERZE znaczy
// zawsze przeplot, niezaleznie od otaczajacych spacji.
//
// Do 2026-08-29 stala tu regula `'# ' ~[\r\n]*`, ktorej autor byl swiadom kolizji
// ("there must be space after the hash - otherwise a#b is recognized as a #comment").
// Odstep po `#` nie rozwiazywal jej jednak, tylko przenosil skutek w cisza: `FROM a # b`
// przechodzilo kompilacje jako `FROM a`, gubiac `b` BEZ zadnego komunikatu. Po usunieciu
// reguly ten sam zapis jest przeplotem, a `SELECT ... # komentarz na koncu wiersza` jest
// bledem skladni — glosnym, a nie cichym. Komentarz konczacy wiersz zapisuje sie `//`.
LINE_COMMENT2:      '//' ~[\r\n]* -> channel(HIDDEN);

fragment LETTER:    [A-Z_];
fragment DEC_DOT_DEC: (DEC_DIGIT+ '.' DEC_DIGIT+ |  DEC_DIGIT+ '.' | '.' DEC_DIGIT+);
fragment HEX_DIGIT: [0-9A-F];
fragment DEC_DIGIT: [0-9];