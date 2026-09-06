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
/// wszystkich komend odpytujących serwer — `select`, `dir`/`dirYaml`, `detailShow*` — bo stan
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
  serverStopping       ///< serwer odpowiedział, ale właśnie się zamyka
};

/// Nazwa trybu do komunikatu dla operatora.
const char *toString(selectResult result);

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

  /// serverName pusta => serwer jednoinstancyjny (nazwy historyczne); niepusta => instancja
  /// o tej nazwie, czyli jej wlasny obszar IPC.
  explicit qry(int serverNoDataTimeoutMs     = kDefaultServerNoDataTimeoutMs,
               int clientResponseMaxFails    = kDefaultClientResponseMaxFails,
               int responseQueueOpenMaxFails = kIpcClientDefaultResponseQueueOpenMaxFails, std::string_view serverName = {});
  selectResult select(boost::program_options::variables_map &vm, int /*iElemLimit*/, const std::string & /*input*/,
                      std::tuple<int, int, int> /*gnuplotDim*/, bool /*gnuplotRightToLeft*/ = false);
  bool adhoc(const std::string & /*sAdhoc*/);

  /// Przeladowanie CALEGO planu instancji trescia @p planText. Zwraca true przy odmowie
  /// (spojnie z adhoc()). Tekst pusty jest zadaniem poprawnym: sprowadza instancje do stanu
  /// bezczynnego. Powod odmowy — komunikat serwera — trafia na stderr, bo jest to jedyna
  /// rzecz, ktora operator moze z ta odmowa zrobic.
  bool reset(const std::string & /*planText*/);

  /// Lista strumieni albo werdykt, dlaczego jej nie ma. Instancja bezczynna ma tu WARTOSC
  /// (wlasny, niepusty wydruk), bo odpowiedziala — porazka jest zarezerwowana dla serwera,
  /// ktory listy nie dostarczyl.
  std::expected<std::string, selectResult> dir();
  std::expected<std::string, selectResult> dirYaml();
  int hello();
  /// Wydruk detalu strumienia albo werdykt. Pusty wydruk nie odrozniał nieznanej nazwy od
  /// milczacego serwera i od instancji bez planu — wszystkie trzy konczyly sie tym samym
  /// kodem wyjscia i bez slowa na stderr, mimo ze kazdy z nich to inna naprawa.
  std::expected<std::string, selectResult> detailShow(const std::string & /*input*/);
  std::expected<std::string, selectResult> detailShowYaml(const std::string & /*input*/);
  virtual boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/);
  virtual ~qry();
};
