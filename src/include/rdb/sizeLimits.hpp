#pragma once

#include <cstddef>
#include <cstdint>

/// Granice wymiarow planu - jedno zrodlo dla gramatyki RQL (RQLParser.cpp), gramatyki DESC
/// (DESCParser.cc) i kompilatora (compiler.cpp).
///
/// Tekst planu przychodzi takze kanalem ad-hoc i `xqry --reset`, czyli do procesu DZIALAJACEGO
/// serwera. Do 2026-09-27 wymiar byl ograniczony wylacznie zakresem int, wiec literal absurdalny jako
/// rozmiar konczyl sie tam std::bad_alloc, OOM killerem albo przepelnieniem int (A2 M11). Granice
/// maja zapas wzgledem planow repozytorium (najwieksza tablica 257, generator 4, okno AGSE 180,
/// DUMP RETENTION 100) i korpusu uc01..uc08, sprawdzonego tego samego dnia.
namespace rdb::limits {

// --- Warstwa 1: pojedynczy literal, sprawdza parser (RQL i DESC) ---

/// Liczba elementow pola: `TYP[N]`, `STRING[N]` (znaki) i szerokosc `to_string(x : N)`, ktora jest
/// dlugoscia pola STRING. Kompilator pilnuje tej granicy takze dla pol POCHODNYCH (konkatenacja
/// napisow), bo serwer zapisuje je do .desc i czyta przy nastepnym starcie.
inline constexpr int kMaxFieldLength = 65536;

/// Zasieg w historii: krok i szerokosc okna AGSE `@(step, window)`, przesuniecie `>N`, okno
/// rekordowe `MIN(x : N)` z rodzenstwem i granice `DUMP -L TO R`. Kazda z tych liczb przeklada sie
/// na pojemnosc historii zrodla, a szerokosc okna AGSE - takze na szerokosc rekordu.
inline constexpr int kMaxHistoryReach = 65536;

/// `DUMP ... RETENTION n`: regula ma w locie najwyzej n zadan zrzutu (po jednym na plik slotu), a kazde
/// otwarte zadanie trzyma deskryptor pliku. 256 to miekki RLIMIT_NOFILE na macOS.
inline constexpr int kMaxDumpRetention = 256;

/// Liczba strumieni planu: rozmiar generatora `STREAM x[N]` (parser) i plan po rozwinieciu
/// generatorow (kompilator). Te sama liczbe bierze uklad magistrali (bus::kMaxStreams), wiec zmiana
/// tutaj zmienia uklad segmentu xrdbbus.
///
/// 148 od 2026-10-02 (wczesniej 128): najwiekszy plan korpusu artykulu (480 planow: .rql z pinu
/// rdb-experiment i K6c w 6 skalach) to K6c W4_Q32 - 131 strumieni w kazdej skali, przy 128 serwer
/// odmawial startu. Nastepny w korpusie ma 96, wiec zapas wynosi 17.
inline constexpr std::size_t kMaxPlanStreams = 148;

/// Termin odczytu zrodla DEVICE w sekundach: klauzula `TIMEOUT` (parser) i `[sources] timeout_s`
/// (retractor.toml), #347. Doba to i tak wiecej niz najdluzszy interwal, przy ktorym czekanie ma sens;
/// granica trzyma termin daleko od przepelnienia zegara monotonicznego w nanosekundach.
inline constexpr double kMaxDeviceTimeoutSeconds = 86400.0;

// --- Warstwa 2: wielkosci zlozone, sprawdza kompilator; biblioteka rdb ich nie stosuje ---

/// Rozmiar jednego rekordu w bajtach.
inline constexpr std::int64_t kMaxRecordBytes = std::int64_t{1} << 20;

/// Suma elementow plaskich wszystkich wezlow planu. Koszt kompilacji i pracy rosnie z liczba
/// elementow, a nie z bajtami: 2026-09-27 zmierzono ok. 390 B na element przy kompilacji i ok. 560 B
/// w pracy, wiec rekord 1 MiB z BYTE (milion elementow) kosztowal ~400 MB na kazdy wezel `SELECT *`.
/// 2^18 to w najgorszym razie ~100 MB kompilacji i ~150 MB pracy.
inline constexpr std::int64_t kMaxPlanElements = std::int64_t{1} << 18;

}  // namespace rdb::limits
