# Opis przypadku testowego

Uzupełnienie testu `issue96_substrat_reference`.

Dwa zapytania użytkownika (`str1`, `str2`) mają identyczną strukturę:
ten sam strumień źródłowy (`core0`) i tę samą operację przesunięcia czasowego (`>2`).

Oba są zdefiniowane jawnie przez użytkownika — żadne nie jest wygenerowanym substratem
(`isSubstrat = false`). Funkcja `deduplicateSubstrats` pomija takie zapytania
(`if (!it->isSubstrat) continue`), więc oba muszą pozostać w planie.

Oczekiwany plan realizacji zapytania:

```
str1(1/10)
        :- PUSH_STREAM(core0)
        :- STREAM_TIMEMOVE(2)
        str1_0: INTEGER
                PUSH_ID(str1[0])
str2(1/10)
        :- PUSH_STREAM(core0)
        :- STREAM_TIMEMOVE(2)
        str2_0: INTEGER
                PUSH_ID(str2[0])
core0(1/10)     datafile1.dat
        a: INTEGER
```
