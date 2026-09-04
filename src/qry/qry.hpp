#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "formatters.hpp"
#include "ipcClient.hpp"

inline constexpr int kDefaultServerNoDataTimeoutMs{10'000};
inline constexpr int kDefaultClientResponseMaxFails{kIpcClientDefaultResponseMaxFails};

/// Wynik `qry::select`. Rozróżnia tryby porażki, bo wszystkie trzy dawały
/// wcześniej albo kod 0, albo mylący komunikat o innym błędzie (issue_215).
///
/// Zasada: klient, który nie przeczytał ani jednego elementu, NIGDY nie kończy
/// się sukcesem. Cichy sukces bez danych wygląda w harnessie i w CI dokładnie
/// tak samo jak poprawny przebieg.
enum class selectResult : std::uint8_t {
  ok,                  ///< strumień czytany; przeczytano co najmniej jeden element
  streamNotFound,      ///< serwer odpowiedział, ale nie zna tego strumienia
  serverNoResponse,    ///< serwer nie odpowiedział na komendę w wyznaczonym czasie
  clientQueueMissing,  ///< serwer nie utworzył kolejki odpowiedzi tego klienta
  noData               ///< dołączono do strumienia, ale nie przyszedł ani jeden element
};

/// Nazwa trybu do komunikatu dla operatora.
const char *toString(selectResult result);

class qry {
  int elemLimitCnt{0};
  int serverNoDataTimeoutMs_{kDefaultServerNoDataTimeoutMs};
  std::map<std::string, boost::property_tree::ptree> streamTable;
  std::unique_ptr<IpcClient> transport_;
  std::unique_ptr<Formatter> formatter_;

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
  std::string dir();
  std::string dirYaml();
  int hello();
  std::string detailShow(const std::string & /*input*/);
  virtual boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/);
  virtual ~qry();
};
