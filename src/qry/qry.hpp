#pragma once
#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "formatters.hpp"
#include "ipcClient.hpp"

inline constexpr int kDefaultServerNoDataTimeoutMs{10'000};

/// Rozmiar porcji, na ktore dzielony jest plan wysylany komenda `--reset`.
///
/// Kolejka komend przyjmuje komunikat do ipc::kQueryQueueMaxMessageSize (1000 B), a wysylany
/// jest nie sam tekst, tylko dokument `info` z nim w srodku: kazdy znak nowej linii, cudzyslow
/// i backslash zajmuja tam po dwa bajty. 400 bajtow surowego tekstu miesci sie w limicie
/// nawet przy najgorszym mozliwym rozstrzeleniu, z zapasem na klucze dokumentu.
inline constexpr std::size_t kResetChunkBytes{400};
inline constexpr int kDefaultClientResponseMaxFails{kIpcClientDefaultResponseMaxFails};

/// Werdykt komendy wysłanej do serwera. Rozróżnia tryby porażki, bo wszystkie trzy dawały
/// wcześniej albo kod 0, albo mylący komunikat o innym błędzie (issue_215). Wspólny dla
/// wszystkich komend odpytujących serwer - `select`, `dir`/`dirYaml`, `detailShow*` - bo stan
/// serwera nie zależy od tego, która komenda go zastała, więc jego nazwa i kod wyjścia też
/// nie mogą.
///
/// Zasada: klient, który nie przeczytał ani jednego elementu, NIGDY nie kończy
/// się sukcesem. Cichy sukces bez danych wygląda w harnessie i w CI dokładnie
/// tak samo jak poprawny przebieg.
enum class selectResult : std::uint8_t {
  ok,                  ///< strumień czytany; przeczytano co najmniej jeden element
  streamNotFound,      ///< serwer odpowiedział, ale nie zna tego strumienia
  serverNoResponse,    ///< serwer nie odpowiedział na komendę w wyznaczonym czasie
  clientQueueMissing,  ///< serwer nie utworzył kolejki odpowiedzi tego klienta
  noData,              ///< dołączono do strumienia, ale nie przyszedł ani jeden element
  noActivePlan,        ///< serwer odpowiedział, ale nie ma wczytanego planu (tryb bezczynny)
  serverStopping,      ///< serwer odpowiedział, ale właśnie się zamyka
  renderFailed         ///< pętla odbioru i renderowania przerwana wyjątkiem po stronie klienta
};

/// Funkcja toString powinna:
/// - zwracać stały opis wyniku selectResult w języku angielskim do komunikatu dla operatora: "ok", "stream unknown to the server", "server did not answer within the timeout", "server did not create the client response queue", "no data in stream", "server has no plan loaded (idle); load one with --reset", "server is shutting down" i "select loop failed in the client; reason in the client log".
/// - dla wartości spoza wyliczenia zwracać "unknown".
const char *toString(selectResult result);

/// Obiekt klasy qry powinien:
/// - rozmawiać z serwerem o podanej nazwie przez IpcClient, przyjmując limit ciszy serwera i budżety prób nie mniejsze niż 1.
/// - rozpoznawać odpowiedź serwera w kolejności: error.response jako brak odpowiedzi, db.stream jako listę strumieni, db równe odpowiedzi braku planu jako stan bezczynny, db równe odpowiedzi zamykania jako zamykanie serwera, a każdą inną jako odpowiedź nieprawidłową raportowaną jak brak odpowiedzi.
/// - w select subskrybować strumień komendą show przed jakąkolwiek inną komendą, a dopiero potem sprawdzać przez get istnienie strumienia, tak aby serwer wstrzymany bramką nie emitował wierszy przed utworzeniem kolejki klienta.
/// - w select zwracać streamNotFound dla strumienia nieznanego serwerowi, noActivePlan, serverStopping albo serverNoResponse według odpowiedzi na get, a clientQueueMissing, gdy serwer odrzucił subskrypcję istniejącego strumienia.
/// - w select renderować elementy wybranego strumienia formatem outputFormatMode do wyczerpania limitu elementów, końca strumienia oznaczonego zarezerwowanym identyfikatorem, ciszy serwera dłuższej niż limit albo przerwania przez operatora.
/// - zatrzymywać select klawiszem wyłącznie w przebiegu bez limitu elementów i bez wymogu Ctrl+C, a przy limicie N renderować dokładnie N elementów także wtedy, gdy producent dostarczył ich więcej naraz.
/// - po zakończeniu pętli select zatrzymywać i dołączać wątek producenta przed ustaleniem werdyktu.
/// - zwracać z select renderFailed przy wyjątku w pętli odbioru, clientQueueMissing albo noData, gdy nie wyrenderowano żadnego elementu, serverNoResponse, gdy serwer zamilkł po części elementów, a ok wyłącznie po co najmniej jednym elemencie bez tych zdarzeń.
/// - przez adhoc wysyłać zapytanie ad hoc i zwracać prawdę przy pustym zapytaniu albo odpowiedzi innej niż OK.
/// - przez reset dzielić tekst planu na porcje po kResetChunkBytes bajtów, wysyłać reset-begin z liczbą porcji, kolejne reset-chunk i reset-commit, przerywać przy pierwszej odpowiedzi innej niż OK z jednym komunikatem na stderr wskazującym etap i powód, i zwracać prawdę przy odmowie; pusty plan wysyłać jako zero porcji, co przy przyjęciu przeprowadza instancję w stan bezczynny.
/// - przez hello zwracać sukces wyłącznie przy odpowiedzi world, a w przeciwnym razie protocol_error.
/// - przez dir zwracać tabelę strumieni z nazwą, czasem, rozmiarem, liczbą rekordów, lokalizacją i pojemnością, a przez dirYaml dokument YAML z nazwą, czasem, liczbą rekordów, rozmiarem pomijanym przy wartości -1 i lokalizacją pomijaną, gdy jest pusta; dla instancji bezczynnej zwracać wydruk braku planu albo dokument YAML z pustą listą, a dla braku odpowiedzi lub zamykania werdykt zamiast wydruku.
/// - przez detailShow i detailShowYaml zwracać nazwę, okres, zapytanie oraz pola strumienia z typami, a dla nieznanego strumienia, instancji bezczynnej, zamykania lub braku odpowiedzi, także przy przeładowaniu planu między get i detail, odpowiedni werdykt zamiast wyjątku.
/// - przez jsonCommand wypisywać na stdout zdarzenia JSON w wersji 1, jedno na wiersz: pong dla hello, streams dla dir, schema dla detail, a dla select schema, kolejne record z wartościami i null oraz end z przyczyną limit albo server_stopped_or_reloaded, a każdą porażkę zgłaszać zdarzeniem error z kodem i kodem wyjścia 1.
class qry {
  int elemLimitCnt{0};
  int serverNoDataTimeoutMs_{kDefaultServerNoDataTimeoutMs};
  std::map<std::string, boost::property_tree::ptree> streamTable;
  std::unique_ptr<IpcClient> transport_;
  std::unique_ptr<Formatter> formatter_;

  /// Odpowiedz serwera na 'detail' albo werdykt, dlaczego jej nie ma. Wspolna dla obu form
  /// wydruku detalu, bo rozstrzygniecie "jest / nie ma" nalezy do serwera i nie moze zalezec
  /// od wybranego formatu.
  std::expected<boost::property_tree::ptree, selectResult> detailNode(const std::string & /*input*/);

 public:
  formatMode outputFormatMode{formatMode::RAW};
  bool gnuplotRightToLeft{false};
  bool gnuplotOhlc{false};

  /// serverName pusta => serwer jednoinstancyjny (nazwy historyczne); niepusta => instancja
  /// o tej nazwie, czyli jej wlasny obszar IPC.
  explicit qry(int serverNoDataTimeoutMs     = kDefaultServerNoDataTimeoutMs,
               int clientResponseMaxFails    = kDefaultClientResponseMaxFails,
               int responseQueueOpenMaxFails = kIpcClientDefaultResponseQueueOpenMaxFails, std::string_view serverName = {});
  selectResult select(boost::program_options::variables_map &vm, int /*iElemLimit*/, const std::string & /*input*/,
                      std::tuple<int, int, int> /*gnuplotDim*/, bool /*gnuplotRightToLeft*/ = false);
  int jsonCommand(const std::string &command, const std::string &input, int limit, int idleTimeoutMs);
  bool adhoc(const std::string & /*sAdhoc*/);

  /// Przeladowanie CALEGO planu instancji trescia @p planText. Zwraca true przy odmowie
  /// (spojnie z adhoc()). Tekst pusty jest zadaniem poprawnym: sprowadza instancje do stanu
  /// bezczynnego. Powod odmowy - komunikat serwera - trafia na stderr, bo jest to jedyna
  /// rzecz, ktora operator moze z ta odmowa zrobic.
  bool reset(const std::string & /*planText*/);

  /// Lista strumieni albo werdykt, dlaczego jej nie ma. Instancja bezczynna ma tu WARTOSC
  /// (wlasny, niepusty wydruk), bo odpowiedziala - porazka jest zarezerwowana dla serwera,
  /// ktory listy nie dostarczyl.
  std::expected<std::string, selectResult> dir();
  std::expected<std::string, selectResult> dirYaml();
  int hello();
  /// Wydruk detalu strumienia albo werdykt. Pusty wydruk nie odrozniał nieznanej nazwy od
  /// milczacego serwera i od instancji bez planu - wszystkie trzy konczyly sie tym samym
  /// kodem wyjscia i bez slowa na stderr, mimo ze kazdy z nich to inna naprawa.
  std::expected<std::string, selectResult> detailShow(const std::string & /*input*/);
  std::expected<std::string, selectResult> detailShowYaml(const std::string & /*input*/);
  virtual boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/);
  virtual ~qry();
};
