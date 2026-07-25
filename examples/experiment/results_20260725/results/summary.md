# Wyniki eksperymentu SDF/CSDF — 2026-07-25

- commit silnika: `e189d0e`
- python: 3.14.4, host: Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.43
- wygenerowano: 2026-07-25T19:59:08+00:00
- binarka użyta w moście: `/home/michal/github/retractordb/build/Debug/src/retractor/xretractor`

## 1. Kontrole mutacyjne

Warunek wstępny: macierz musi wykrywać wstrzyknięty błąd.

| mutacja | oczekiwana detekcja | wykryta na | werdykt |
|---|---|---:|---|
| `beatty_off_by_one` | tak | 12/12 | OK |
| `beatty_a_index` | tak | 12/12 | OK |
| `oracle_tie_to_b` | tak | 11/12 | OK |
| `csdf_period_minus_one` | tak | 11/12 | OK |
| `unreduced_ratio_is_benign` | nie (kontrola negatywna) | 0/12 | OK |

## 2. Zgodność oracle — modele

| kampania | modele | przypadków | porównanych pozycji | rozbieżności | błędy okresu |
|---|---|---:|---:|---:|---:|
| exhaustive<=64 | 4 | 4096 | 802888 | 0 | 0 |
| random<=256 | 4 | 3000 | 1697808 | 0 | 0 |
| random<=1e6 | 1 | 800 | 1600000 | 0 | 0 |
| special | 4 | 12 | 6042 | 0 | 0 |
| **razem** | | **7908** | **4106738** | **0** | **0** |

Nieskrócony zapis stosunku daje ten sam ślad co skrócony: `True`.

## 3. Most oracle — silnik RetractorDB

| przypadek | Δa | Δb | a/b | P | Δc | porównano | okresów | prefiks zer | wynik |
|---|---|---|---|---:|---|---:|---:|---:|---|
| p3_engine_test | 1/10 | 1/5 | 1/2 | 3 | 1/15 | 44 | 14.67 | 0 | OK |
| p2_equal | 1/4 | 1/4 | 1/1 | 2 | 1/8 | 59 | 29.5 | 0 | OK |
| p8_coprime | 1/5 | 1/3 | 3/5 | 8 | 1/8 | 68 | 8.5 | 0 | OK |
| p9_skewed | 1/2 | 1/7 | 7/2 | 9 | 1/9 | 112 | 12.44 | 0 | OK |
| p17_coprime | 1/10 | 1/7 | 7/10 | 17 | 1/17 | 159 | 9.35 | 0 | OK |
| p3_unreduced | 1/10 | 1/5 | 1/2 | 3 | 1/15 | 44 | 14.67 | 0 | OK |
| p103_skewed | 3/100 | 1 | 3/100 | 103 | 3/103 | 201 | 1.95 | 0 | OK |
| p307_audio | 4/25 | 147/1000 | 160/147 | 307 | 588/7675 | 1151 | 3.75 | 0 | OK |

## 4. Koszt reprezentacji

Wielkości strukturalne, wyprowadzone z definicji modeli — nie pomiary czasu.

**Δa/Δb = 1/2, P = 3**

| reprezentacja | fazy | słowa opisu | startup (tokeny) | bufor wejść |
|---|---:|---:|---:|---:|
| beatty_online | 1 | 3 | 1 | 1 |
| csdf_explicit | 3 | 3 | 1 | 1 |
| csdf_lookup | 3 | 3 | 1 | 1 |
| sdf_block | 1 | 5 | 3 | 3 |

**Δa/Δb = 7/10, P = 17**

| reprezentacja | fazy | słowa opisu | startup (tokeny) | bufor wejść |
|---|---:|---:|---:|---:|
| beatty_online | 1 | 3 | 1 | 1 |
| csdf_explicit | 17 | 17 | 1 | 1 |
| csdf_lookup | 17 | 17 | 1 | 1 |
| sdf_block | 1 | 19 | 17 | 17 |

**Δa/Δb = 3/100, P = 103**

| reprezentacja | fazy | słowa opisu | startup (tokeny) | bufor wejść |
|---|---:|---:|---:|---:|
| beatty_online | 1 | 3 | 1 | 1 |
| csdf_explicit | 103 | 103 | 1 | 1 |
| csdf_lookup | 103 | 103 | 1 | 1 |
| sdf_block | 1 | 105 | 103 | 103 |

**Δa/Δb = 160/147, P = 307**

| reprezentacja | fazy | słowa opisu | startup (tokeny) | bufor wejść |
|---|---:|---:|---:|---:|
| beatty_online | 1 | 3 | 1 | 1 |
| csdf_explicit | 307 | 307 | 1 | 1 |
| csdf_lookup | 307 | 307 | 1 | 1 |
| sdf_block | 1 | 309 | 307 | 307 |

