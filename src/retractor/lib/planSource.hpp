#pragma once

#include <string>
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

/// Normalizacja sciezki publikowanej w slocie magistrali. absolute() PRZED weakly_canonical():
/// plik licznika przy pierwszym starcie jeszcze nie istnieje, a weakly_canonical nad
/// nieistniejaca sciezka wzgledna zwraca ja bez zmiany — czyli bez katalogu roboczego,
/// o ktory w tej normalizacji chodzi.
[[nodiscard]] std::string absolutePathOf(const std::string &path);
