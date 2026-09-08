#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler.hpp"
#include "qTree.hpp"

/// @brief Wczytanie zestawu RQL z TEKSTU i sprzatanie artefaktow poprzedniego planu.
///
/// Obie czynnosci sa wspolne dla dwoch drog, ktorymi plan trafia do silnika: startu
/// z pliku (launcher) i przeladowania w locie komenda `reset` (executorsm). Rozjazd
/// miedzy nimi bylby rozjazdem SEMANTYKI planu — inaczej rozpoznany komentarz albo
/// nieskasowany plik `.desc` po starym strumieniu — wiec zrodlo jest jedno.

/// Wynik wczytania zestawu.
struct PlanSource {
  /// "OK" albo komunikat parsera dla pierwszej instrukcji, ktora sie nie powiodla.
  std::string status{"OK"};
  /// Pary (nazwa strumienia z zapisu, tresc instrukcji) w kolejnosci wystapienia.
  /// Zasila `processedLines` — diagnostyke `xqry -t` i sprzatanie artefaktow.
  std::vector<std::pair<std::string, std::string>> lines;
};

/// Parsuje caly zestaw RQL podany jako tekst i dopisuje jego wezly do @p plan.
///
/// Tekst PUSTY (albo zlozony wylacznie z komentarzy i pustych wierszy) nie jest bledem:
/// status zostaje "OK", a lista instrukcji pusta. Rozstrzygniecie, czy pusty plan znaczy
/// "tryb bezczynny", czy "nie ma czego kompilowac", nalezy do wolajacego — dla uslugi to
/// stan poprawny, dla `--onlycompile` blad.
[[nodiscard]] PlanSource parsePlanText(qTree &plan, const std::string &text);

/// Kasuje artefakty (`<id>`, `<id>.desc`, `<id>.meta`) strumieni powolanych przez @p lines.
///
/// Nic nie robi, gdy plan niesie `:ROTATION` — tam poprzednie przebiegi sa danymi, nie
/// smieciem. Rodziny rozwiniete przez generator bierze z @p cm, bo nazwa z zapisu nie
/// musi byc nazwa w planie (patrz compiler::generatedStreams()).
void dropStalePlanArtifacts(qTree &plan, const compiler &cm, const std::vector<std::pair<std::string, std::string>> &lines);

/// Nazwy strumieni, ktore plan ROSCI na magistrali: wszystkie wezly poza dyrektywami.
[[nodiscard]] std::vector<std::string> planStreamNames(const qTree &plan);

/// Znormalizowana sciezka licznika `:ROTATION`; pusta, gdy plan nie ma rotacji.
[[nodiscard]] std::string planCounterPath(const qTree &plan);

/// Katalog magazynu, ktorego plan naprawde uzyje: dyrektywa `:STORAGE`, a gdy jej nie ma --
/// @p defaultStorageDir (domyslny katalog z konfiguracji). Pusty wynik znaczy "katalog roboczy".
[[nodiscard]] std::string planStorageDir(const qTree &plan, std::string_view defaultStorageDir);

/// Znormalizowane sciezki PLIKOW MAGAZYNU, ktore plan ROSCI na magistrali.
///
/// Nazwa strumienia nie chroni pliku danych, a wylacznie deskryptor `<id>.desc` -- tylko on jest
/// budowany z identyfikatora zapytania. Sam plik danych bierze `qry.filename`, gdy zapytanie ma
/// klauzule `FILE` (patrz streamInstance), wiec dwa plany o ROZLACZNYCH nazwach moga wskazywac
/// jeden magazyn.
///
/// Ze zbioru wypadaja dwa rodzaje wezlow, i oba wypadaja z powodu, nie dla wygody:
///   - DEKLARACJE: `query::descriptorStorage()` nadaje kazdej `TYPE TEXTSOURCE` albo `DEVICE`,
///     czyli zrodlo TYLKO DO ODCZYTU. Wiele serwerow czytajacych jeden plik jest poprawne
///     (na tym stoi caly it_multiserver_uniqueness, gdzie kilka planow czyta `data.txt`).
///   - MEMORY (`VOLATILE` albo `STORAGE memory`): `rdb::memoryFile` zyje w pamieci procesu
///     i nie dotyka systemu plikow, wiec roszczenie sciezki byloby konfliktem o nic.
///
/// @param defaultStorageDir katalog uzywany, gdy plan nie niesie dyrektywy `:STORAGE` -- ta sama
///        wartosc, ktora launcher dokleda z konfiguracji (`[storage] dir`). Bez niej rezerwacja
///        wskazywalaby katalog roboczy, a plan pisalby gdzie indziej.
///
/// ZNANY LIMIT: pole `REF` w istniejacym `.desc` przestawia plik magazynu poza katalog `:STORAGE`
/// (rdb::StoragePaths::relocateFromRef). Z samego planu tego nie widac, a czytanie deskryptora
/// kazdego strumienia przy kazdym roszczeniu byloby operacja dyskowa w sciezce, ktora ma byc
/// atomowa i krotka. Sciezki po `REF` pozostaja wiec niechronione.
[[nodiscard]] std::vector<std::string> planStorePaths(const qTree &plan, std::string_view defaultStorageDir);

/// Normalizacja sciezki publikowanej w slocie magistrali. absolute() PRZED weakly_canonical():
/// plik licznika przy pierwszym starcie jeszcze nie istnieje, a weakly_canonical nad
/// nieistniejaca sciezka wzgledna zwraca ja bez zmiany — czyli bez katalogu roboczego,
/// o ktory w tej normalizacji chodzi.
[[nodiscard]] std::string absolutePathOf(const std::string &path);
