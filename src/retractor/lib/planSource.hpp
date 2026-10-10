#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fatalError.hpp"
#include "qTree.hpp"

/// @brief Wczytanie zestawu RQL z TEKSTU i sprzatanie artefaktow poprzedniego planu.
///
/// Obie czynnosci sa wspolne dla dwoch drog, ktorymi plan trafia do silnika: startu
/// z pliku (launcher) i przeladowania w locie komenda `reset` (executorsm). Rozjazd
/// miedzy nimi bylby rozjazdem SEMANTYKI planu - inaczej rozpoznany komentarz albo
/// nieskasowany plik `.desc` po starym strumieniu - wiec zrodlo jest jedno.

/// Wynik wczytania zestawu.
struct PlanSource {
  /// "OK" albo komunikat parsera dla pierwszej instrukcji, ktora sie nie powiodla.
  std::string status{"OK"};
  /// Pary (nazwa strumienia z zapisu, tresc instrukcji) w kolejnosci wystapienia.
  /// Zasila `processedLines` - diagnostyke `xqry -t` i sprzatanie artefaktow.
  std::vector<std::pair<std::string, std::string>> lines;
};

/// Parsuje caly zestaw RQL podany jako tekst i dopisuje jego wezly do @p plan.
///
/// Tekst PUSTY (albo zlozony wylacznie z komentarzy i pustych wierszy) nie jest bledem:
/// status zostaje "OK", a lista instrukcji pusta. Rozstrzygniecie, czy pusty plan znaczy
/// "tryb bezczynny", czy "nie ma czego kompilowac", nalezy do wolajacego - dla uslugi to
/// stan poprawny, dla `--onlycompile` blad.
/// `sourceFile` jest opcjonalna nazwa pliku dla logu bledow skladni.
[[nodiscard]] PlanSource parsePlanText(qTree &plan, const std::string &text, std::string_view sourceFile = {});

/// Kasuje pelne rodziny plikow (dane z cieniem, `.desc`, `.meta` z cieniem, segmenty retencji)
/// wszystkich wezlow skompilowanego planu, takze posrednich.
///
/// Przy `:ROTATION` wezly plikowe zostaja - tam poprzednie przebiegi sa danymi, nie smieciem -
/// a znika tylko konfiguracja strumieni MEMORY. Katalog to `:STORAGE` planu (z domyslnym
/// katalogiem konfiguracji juz dopisanym przez wolajacego).
void dropStalePlanArtifacts(const qTree &plan);

/// Zgodnosc plikow, ktore `:ROTATION` zachowa, z planem: TYPE i RETENTION z zachowanego `.desc`
/// wygrywaja w magazynie z planem, a zmiana pojemnosci przy zachowanych segmentach przestawilaby
/// adresowanie rekordow (groupFile liczy segment jako indeks / capacity). Dlatego rozjazd jest
/// odmowa planu, zanim cokolwiek zostanie zmienione (decyzja P5), a nie cicha wygrana jednej strony.
///
/// Pusty message oznacza sukces. Bez `:ROTATION` start kasuje rodziny plikow.
[[nodiscard]] rdb::Refusal checkKeptStores(qTree &plan, std::string_view defaultStorageDir,
                                           const std::vector<std::string> &streamNames = {});

/// Sprawdza zachowane pliki .desc przed zmiana planu: deklaracje zawsze, wyniki tylko przy ROTATION.
/// Deklaracja musi sie zgadzac takze w TYPE i REF, bo te magazyn bierze z pliku, nie z planu (#346).
/// Jedyny przepuszczany rozjazd to dawny TYPE DEVICE zwyklego pliku binarnego, ktory zastepuje
/// dropStalePlanArtifacts(). Pusta lista nazw oznacza caly plan; ad-hoc przekazuje tylko nowo
/// dodawane strumienie, ktorych deskryptory zostaja takze bez ROTATION i dla MEMORY.
[[nodiscard]] rdb::Refusal checkDescriptorFiles(qTree &plan, std::string_view defaultStorageDir,
                                                const std::vector<std::string> &streamNames = {});

/// Rodzaj pliku pod sciezka kazdej deklaracji (#346): BINFILE i TEXTFILE - plik zwykly, DEVICE -
/// urzadzenie znakowe albo FIFO. Tylko stat(), bez otwarcia, wiec FIFO bez pisarza niczego nie
/// wiesza. Pusta lista nazw oznacza caly plan; ad-hoc przekazuje tylko nowo dodawane strumienie.
/// Nie w `-c` - kompilacja nie musi biec na maszynie z danymi.
/// Zwraca odmowe z nazwa strumienia i sciezka; przy formie przestarzalej `FILE`
/// takze z podpowiedzia jawnego slowa.
[[nodiscard]] rdb::Refusal checkDeclaredSources(const qTree &plan, const std::vector<std::string> &streamNames = {});

/// STORAGE musi istniec i byc katalogiem, takze gdy wszystkie strumienie sa MEMORY.
[[nodiscard]] rdb::Refusal checkStorageDirectory(const qTree &plan, std::string_view defaultStorageDir);

/// Wspolny preflight czterech drog (#473), przed kasowaniem plikow/importem/rezerwacja.
/// Pusta lista oznacza wymiane calego planu; lista nazw - nowe strumienie ad-hoc.
[[nodiscard]] rdb::Refusal checkPlanStorage(qTree &plan, std::string_view defaultStorageDir,
                                            const std::vector<std::string> &streamNames = {});

/// Wynik checkRotationCounter(): pusty `refusal` = plan bez `:ROTATION` albo licznik uzyteczny.
struct CounterCheck {
  bool environment{false};  ///< true: pliku nie da sie sprawdzic ani odczytac (K4); false: wadliwa tresc (K2)
  std::string refusal;
};

/// Licznik `:ROTATION` przed zmiana planu (#472): ta sama odmowa co checkKeptStores, zanim cokolwiek
/// zostanie zmienione, bo konstruktor PersistentCounter biegnie juz po punkcie bez powrotu. Tylko
/// odczyt - numer rezerwuje dopiero konstruktor. Rozdziela przyczyne, bo start reaguje na nie
/// inaczej: wadliwa tresc to odmowa planu, a nieczytelny plik - awaria srodowiska z planem
/// zachowanym. Nie w `-c` - kompilacja nie musi biec na maszynie z danymi.
[[nodiscard]] CounterCheck checkRotationCounter(const qTree &plan);

/// Ostrzezenia o deklaracjach w przestarzalej formie `DECLARE ... FILE` (#346), po jednym na
/// deklaracje, w kolejnosci wierszy planu - kompilator sortuje wezly po interwale. Kazde nazywa
/// wiersz, strumien i slowo wybrane przez zamrozona regule. Wypisuje je tylko wolajacy z --verbose:
/// bez niego stare plany, testy i narzedzia pomiarowe nie zmieniaja wyjscia. Pusta lista nazw
/// oznacza caly plan; ad-hoc przekazuje tylko nowo dodawane strumienie.
[[nodiscard]] std::vector<std::string> deprecatedFileWarnings(const qTree &plan,
                                                              const std::vector<std::string> &streamNames = {});

/// Ostrzezenia o zrodlach DEVICE, ktorych efektywny TIMEOUT (jawna klauzula albo @p configured
/// z `[sources] timeout_s`) jest dluzszy od interwalu strumienia (#347): czekanie przekroczy wtedy
/// slot. Po jednym na deklaracje, w kolejnosci wierszy planu. Pusta lista nazw oznacza caly plan.
[[nodiscard]] std::vector<std::string> deviceTimeoutWarnings(const qTree &plan, std::optional<double> configured,
                                                             const std::vector<std::string> &streamNames = {});

/// Efektywny termin kazdego zrodla DEVICE wraz z jego pochodzeniem (#347): "RQL", "config",
/// "default" albo "no-clock" (`-f`). Wpis do dziennika przy starcie epoki i przy imporcie ad-hoc -
/// operator widzi, z jakim terminem naprawde czyta kazde zrodlo. Pusta lista nazw oznacza caly plan.
[[nodiscard]] std::vector<std::string> deviceTimeoutReport(const qTree &plan, std::optional<double> configured, bool noClock,
                                                           const std::vector<std::string> &streamNames = {});

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
///   - DEKLARACJE: `query::descriptorStorage()` nadaje kazdej `TYPE BINFILE`, `TEXTSOURCE` albo `DEVICE`,
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

/// Sprawdza otwarcie plikow wyjsciowych SELECT przed publikacja planu. Pusta lista nazw
/// oznacza caly plan; ad-hoc przekazuje tylko nowo dodawane strumienie.
/// K2: wadliwa sciezka/rodzaj pliku; K4: system odmowil operacji (z zachowanym errno).
[[nodiscard]] rdb::Refusal checkOutputFilesOpenable(const qTree &plan, std::string_view defaultStorageDir,
                                                    const std::vector<std::string> &streamNames = {});

/// Normalizacja sciezki publikowanej w slocie magistrali. absolute() PRZED weakly_canonical():
/// plik licznika przy pierwszym starcie jeszcze nie istnieje, a weakly_canonical nad
/// nieistniejaca sciezka wzgledna zwraca ja bez zmiany - czyli bez katalogu roboczego,
/// o ktory w tej normalizacji chodzi.
[[nodiscard]] std::string absolutePathOf(const std::string &path);
