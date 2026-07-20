# Kandydat C3 (ODRZUCONY): small_vector zamiast std::vector<token> arg

**Hipoteza:** `std::vector<token> arg` w `constructInputPayload` alokuje bufor
sterty na każde wywołanie (~15/interwał); `boost::container::small_vector<token,3>`
z inline storage usunie te alokacje (lProgram ma <4 tokeny).

**Pomiar (E1, 5×20000, unpaced, taskset -c 3):**

| wskaźnik | C1+C2 (baseline) | C3 (small_vector) | Δ |
|---|---|---|---|
| p50 compute | 281.7 µs (280.7–284.9) | 282.1 µs (279.4–283.4) | +0.4 µs (w szumie) |
| p99 compute | 762.1 µs | 732.2 µs | nieokreślone (p99 zaszumione) |

**Werdykt: ODRZUĆ.** Pasma p50 pokrywają się — brak mierzalnego zysku. Alokacja
małego bufora vectora nie jest wąskim gardłem (glibc malloc obsługuje małe
bloki tanio, prawdopodobnie z lokalnego cache areny). Zgodnie z zasadą „minimum
code" zmiana cofnięta — dodawała include + kontener bez korzyści.

**Wniosek dla dalszych poszukiwań:** narzut kopii w gorącej ścieżce był
zdominowany przez kopie CAŁEGO obiektu `query` (C1+C2, 35/interwał, wiele list),
nie przez pojedyncze małe alokacje. Kolejni kandydaci powinni celować w większe
struktury (kopie payloadów, ścieżka `constructOutputPayload`/`write`), nie w
mikro-alokacje.
