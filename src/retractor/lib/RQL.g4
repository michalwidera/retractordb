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
                      STREAM stream_name=ID ('[' gen_size=DECIMAL ']')?
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
                    | tablename=ID '[' gen_index ']'             # FieldIDGenerated   // id[$] - ID2
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

// Potegowanie `a^b`. Stoi PIERWSZE, wiec wiaze mocniej niz `*` i `/`: `a*b^2` to `a*(b^2)`.
// Laczne PRAWOSTRONNIE, jak w matematyce — `2^3^2` to `2^(3^2)` = 512, a nie 64. Jest to
// jedyny operator w tej gramatyce, ktory nie jest lewostronny; wolno tak, bo ta drabina
// buduje program ONP dla expressionEvaluator, a nie nazwe substratu (tam lewostronnosc
// jest wymogiem — patrz komentarz przy stream_expression).
//
// UWAGA na jednoargumentowy minus: `-2^2` daje 4, a `-x^2` daje -(x^2). Nie jest to
// niedopatrzenie tylko skutek tego, ze literal ujemny (`'-'? DECIMAL`) jest PRYMITYWEM
// tego samego pietra, a `unary_op_expression` siega po cale `expression`. Dla `*` ta sama
// asymetria nie zmieniala wyniku, dla `^` zmienia — zamiar zapisuje sie nawiasem.
//
// Token to BIT_XOR, zdefiniowany w lekserze od poczatku i do 2026-08-29 nieuzywany w
// zadnej regule parsera. `^` nie koliduje z pozostalymi znakami interpunkcyjnymi RQL
// (`#` przeplot, `&` i `%` rozplot, `@` okno, `$` indeks generatora, `:` szerokosc
// to_string), wiec wybor nie zabiera niczego innego.
term                : <assoc=right> term BIT_XOR term  # ExpPow
                    | term STAR term               # ExpMult
                    | term DIVIDE term             # ExpDiv
                    | '(' expression_factor ')'    # ExpIn
                    | '-'? FLOAT                   # ExpFloat
                    | '-'? DECIMAL                 # ExpDec
                    | STRING                       # ExpString
                    | unary_op_expression          # ExpUnary
                    | field_id                     # ExpField
                    | window_agg                   # ExpWindowAgg
                    | agregator                    # ExpAgg
                    | DOLLAR                       # ExpGenIndex
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
                    | ID '[' gen_index ']'
                    | '(' stream_expression ')'
                    | stream_fn_call
                    ;

// Indeks generatora strumieni: wyrazenie CALKOWITE nad `$` i literalami, zwijane do liczby
// przez compiler::expandStreamGenerators() ZANIM ruszy jakikolwiek inny przebieg. `$` znaczy
// numer porzadkowy instancji, wiec `cells[23-$]` w rodzinie `cell[24]` daje kolejno
// cells[23], cells[22], ... — a po zwinieciu token jest nie do odroznienia od recznie
// napisanego `cells[22]`. Dzieki temu generator nie dotyka ani DAG, ani silnika.
//
// Regula jest CELOWO zamknieta: ewaluator w kompilatorze obsluguje dokladnie te produkcje
// i nic wiecej. Poszerzenie jej tutaj bez poszerzenia ewaluatora konczy sie FatalError.
//
// `$` jest tokenem DOLLAR, nieuzywanym dotad w zadnej regule parsera. Wybrany, bo pozostale
// znaki interpunkcyjne sa zajete przez operatory (`&` rozplot, `%` rozplot modulo, `#` przeplot,
// `@` okno, `_` gwiazdka indeksowa, `:` szerokosc to_string), a `\` sklaja wiersze logiczne
// w readLogicalLines(). Leksuje sie jednoznacznie po `[`, bo ID musi zaczynac sie litera.
gen_index           : gen_index STAR gen_index
                    | gen_index (PLUS | MINUS) gen_index
                    | '(' gen_index ')'
                    | DOLLAR
                    | DECIMAL
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

// Agregat okna REKORDOWEGO w liscie SELECT: `MIN(cells[0] : 10)`.
//
// Drugi argument liczy HISTORYCZNE WIERSZE: okno redukuje DOKLADNIE tyle wartosci, po jednej
// z kazdego z 10 kolejnych rekordow zrodla, i wydaje wynik co rekord — jest PRZESUWNE i innym
// byc nie moze. Redukcja idzie wiec wylacznie po CZASIE.
//
// Do 2026-08-31 argumentem mogla byc cala tablica i wtedy `MIN(cells : 10)` nad `INTEGER[24]`
// redukowalo 240 wartosci, mieszajac czas z 24 ROWNOLEGLYMI kanalami jednego rekordu. Nazwa
// tablicy nie jest nazwa pola — polami sa jej elementy `cells[0]`, `cells[1]`, ... — wiec
// argumentem jest element. Redukcja po kanalach jednego rekordu ma wlasny zapis po stronie
// FROM (`FROM MIN(strumien)`) i pozostaje bez zmian.
//
// KROKU TU NIE MA i miec nie moze. Do 2026-08-31 regula przyjmowala trzeci czlon
// `MIN(cells : 10 : 10)`, ktory wydawal wynik co 10 rekordow, mnozac przez 10 interwal
// strumienia wyjsciowego. Byla to JEDYNA konstrukcja w tym jezyku, w ktorej lista SELECT
// zmieniala takt strumienia; wszedzie indziej interwal wynika wylacznie z klauzuli FROM.
// Kazde okno z krokiem zapisuje sie po stronie FROM — albo przez AGSE `@(krok, szerokosc)`,
// albo przez rozrzedzenie wyniku operatorem `-`. Rozrzedzenie wyniku zachowuje przy tym
// tresc okna (te same W kolejnych rekordow), a rozni sie od dawnego kroku wylacznie faza,
// czyli tym, ktore okno okresu zostaje wydane.
//
// Jest to rodzina ORTOGONALNA wobec `FROM MIN(strumien)`, ktory redukuje pola JEDNEGO
// rekordu i zostaje bez zmian. Roznica jest widoczna w gramatyce: tamten stoi w
// stream_expression i bierze strumien, ten stoi w `term` i bierze POLE.
//
// Separatorem jest COLON, nie COMMA — przecinek rozdziela pozycje `select_list`, a pulapka
// SLL opisana przy tej regule zamienilaby `MIN(a, 10)` w `MIN(a)` plus smiec `10`. Ten sam
// powod i ten sam znak co w `to_string(expr : N)`.
//
// Argumentem jest WYRAZENIE, nie samo odwolanie do pola: `MIN(a[0]*10 - a[1] : 3)` jest
// legalne. Wyrazenie liczy sie osobno dla KAZDEGO rekordu okna, na jego wlasnym payloadzie,
// wiec do redukcji wchodzi po jednej wartosci na rekord — tak samo jak przy golym polu.
// Wszystkie odwolania w wyrazeniu musza siegac po JEDEN strumien: okno czyta historie
// jednego zrodla i innej nie ma skad wziac. Kompilator odrzuca tez okno w oknie oraz
// wyrazenie, ktore nie czyta zadnego pola albo daje napis.
//
// Podwyrazenie WYCHODZI z programu pola w compiler::resolveWindowAggregates() i laduje
// w tabeli grup okna, czyli PRZED przebiegami upraszczajacymi — stale w nim nie sa zwijane.
//
// Alternatywa stoi w `term` PRZED `agregator`, bo `agregator` to sam token MIN i pasuje do
// prefiksu. ANTLR poradzilby sobie predykcja adaptacyjna, ale kolejnosc jest darmowa.
window_agg          : ( MIN
                    | MAX
                    | AVG
                    | SUMC
                    ) '(' expression_factor COLON width=DECIMAL ')'
                    ;

// Wywolanie funkcji skalarnej. Nazwa jest zwyklym ID, a NIE lista literalow: lista
// dozwolonych nazw stoi w `src/include/rqlFunctions.hpp` i sprawdza ja
// compiler::checkFunctionCalls(), wiec `-c` jest bramka.
//
// Do 2026-08-30 nazwy byly literalami tej reguly. Mialo to dwa skutki naraz. Wielkosc
// liter byla czescia SKLADNI — `Sqrt(x)` przechodzilo, `sqrt(x)` bylo bledem, a dla
// `to_integer` i `isnull` odwrotnie — bo ewaluator sklada wielkosc liter przed
// dopasowaniem, a gramatyka nie. I trzynascie nazw bez implementacji kompilowalo sie
// czysto, zeby wywrocic proces dopiero w wykonaniu. Oba znika, gdy nazwa jest ID.
//
// `ID` nie koliduje tu z `field_id : column_name=ID`, bo rozroznia je JEDEN token
// wyprzedzenia (`(` po nazwie); zadna alternatywa `field_id` nie zaczyna sie `ID '('`.
// Pulapka SLL opisana przy `select_list` tego nie dotyczy — tam obie sciezki byly
// poprawne, tu tylko jedna.
//
// Dwa ksztalty wywolania, bo drugi argument `to_string` NIE jest wartoscia na stosie,
// tylko zadeklarowana szerokoscia pola wyjsciowego, i jedzie w tokenie jako IDXPAIR.
// Gdyby stal tu `expression_factor`, jego wlasny token PUSH_VAL zostalby na stosie
// jako smiec, bo CALL2 zdejmuje jeden argument. Separatorem jest COLON, nie COMMA —
// patrz regula przy `select_list`. Liczbe argumentow sprawdza tabela arnosci.
function_call       : fn=ID '(' expression_factor ')'
                    | fn=ID '(' expression_factor COLON DECIMAL ')'
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

// UWAGA: `to_integer`, `to_float`, `to_double` i `to_string` mialy tu do 2026-08-30 wlasne
// tokeny leksera, wiec byly slowami ZASTRZEZONYMI. Teraz leksuja sie jako ID, tak samo jak
// pozostale nazwy funkcji — inaczej nie przeszlyby przez `function_call : fn=ID ...`.
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