#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

/// @brief Pliki blokad flock, ktore wolno kasowac.
///
/// Plik blokady, ktorego nikt nie kasuje, zostaje na dysku na zawsze - po jednym na kazda
/// nazwe instancji. Samo "skasuj plik, ktorego nikt nie trzyma" jest jednak wyscigiem: proces,
/// ktory otworzyl sciezke tuz przed skasowaniem, zajmuje potem blokade na i-wezle bez nazwy,
/// a nastepny tworzy nowy plik i zajmuje go takze - dwoch wlascicieli jednej tozsamosci.
/// Wyscig zamykaja dwie reguly, ktorych przestrzega kazdy uczestnik:
///   1. plik kasuje wylacznie ten, kto trzyma na nim blokade WYLACZNA, i robi to PRZED jej
///      zwolnieniem;
///   2. kto zajmie blokade, sprawdza, czy sciezka nadal wskazuje ten sam i-wezel, a jesli nie,
///      zaczyna od nowa.
/// Blokada zajeta na i-wezle wskazywanym przez sciezke jest wiec jedyna w swoim rodzaju,
/// a porzucony plik moze skasowac kazdy, komu uda sie zajac go wylacznie.
namespace lockfile {

enum class Result : std::uint8_t { Acquired, Busy, Error };

/// Zajmuje blokade pliku `path`, tworzac go w razie potrzeby.
///
/// exclusive: LOCK_EX bez czekania na zywego wlasciciela, ale z krotkim oknem na wlasciciela
///            przejsciowego - sprzatacza albo proces, ktory wlasnie kasuje plik. Inaczej LOCK_SH
///            z czekaniem: blokade wylaczna na pliku obecnosci trzymaja tylko tacy wlasciciele.
/// writable:  deskryptor O_RDWR (plik niesie tresc). Inaczej O_RDONLY - do flock wystarcza,
///            a dziala takze na pliku, do ktorego ten uzytkownik nie ma prawa zapisu.
/// Przy Result::Error errno opisuje przyczyne.
Result acquire(const std::string &path, bool exclusive, bool writable, int &fd);

/// Czy deskryptor nadal odpowiada sciezce (ten sam i-wezel).
[[nodiscard]] bool stillLinked(int fd, const std::string &path);

/// Kasuje plik trzymany blokada wylaczna i zamyka deskryptor, co zwalnia blokade. Pliku, ktory
/// przestal byc tym i-wezlem, nie rusza: pod ta sciezka lezy wtedy cudza blokada.
void removeAndRelease(const std::string &path, int fd);

/// Czy ktos trzyma blokade wylaczna na pliku `path`. Sprawdza przez LOCK_SH bez czekania, wiec
/// wlasciciel startujacy w tej samej chwili trafia co najwyzej na okno wlasciciela przejsciowego.
[[nodiscard]] bool isHeld(const std::string &path);

/// Zajmuje porzucony plik blokady: bez tworzenia i bez czekania. -1, gdy plik jest trzymany,
/// zniknal albo nie da sie go otworzyc.
[[nodiscard]] int claimAbandoned(const std::string &path);

/// Przeglada katalog `dir`. Dla kazdego pliku, ktorego nazwe przyjmie `accept`, zajmuje porzucona
/// blokade, wola `removeGuarded(nazwa)` - wciaz pod ta blokada - i kasuje plik. Zwraca liczbe
/// skasowanych plikow. Pliki trzymane przez zywych wlascicieli zostaja nietkniete.
std::size_t sweep(const std::string &dir, const std::function<bool(std::string_view)> &accept,
                  const std::function<void(std::string_view)> &removeGuarded);

}  // namespace lockfile
