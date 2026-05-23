# Opis problemu

Kompilacja queryWithSubstrats.rql tworzyła następujący plan realizacji zapytania:

(xretractor -c)

STREAM_TIMEMOVE_core0(1/10)
        :- PUSH_STREAM(core0)
        :- STREAM_TIMEMOVE(2)
        a: INTEGER
                PUSH_ID(STREAM_TIMEMOVE_core0[0])
str1(1/10)
        :- PUSH_STREAM(STREAM_TIMEMOVE_core0)
        :- PUSH_STREAM(core1)
        :- STREAM_ADD
        str1_0: INTEGER
                PUSH_ID(str1[0])
str2(1/10)
        :- PUSH_STREAM(core0)
        :- STREAM_TIMEMOVE(2)
        str2_0: INTEGER
                PUSH_ID(str2[0])
core0(1/10)     datafile1.dat
        a: INTEGER
core1(1/5)      datafile2.dat
        a: INTEGER

Oczekujemy że w planie realizacji zapytania substrat STREAM_TIMEMOVE_core0 zostanie zredukowany i zastąpiony przez str2 w odwołaniu PUSH_STREAM(STREAM_TIMEMOVE_core0).
Redukacja może nastąpić wtedy i tylko wtedy kiedy schematy są takie same, delty są takie same oraz polecenia przetwarzania strumieni są też takie same.

Oczekiwany plan realizacji zapytania przedstawia się po redukcji następująco:

str1(1/10)
        :- PUSH_STREAM(str2)
        :- PUSH_STREAM(core1)
        :- STREAM_ADD
        str1_0: INTEGER
                PUSH_ID(str1[0])
str2(1/10)
        :- PUSH_STREAM(core0)
        :- STREAM_TIMEMOVE(2)
        str2_0: INTEGER
                PUSH_ID(str2[0])
core0(1/10)     datafile1.dat
        a: INTEGER
core1(1/5)      datafile2.dat
        a: INTEGER

Test pokrywa ten przypadek.
Usuwane są tylko wytworzone substraty.
Zapytania, które nie zostały wygenerowane nie są redukowane.  