#include <gtest/gtest.h>

#include <unistd.h>

#include <array>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "qry/qry.hpp"

// ctest -R '^ut-xqry' -V

// Single-stream fake - returns the same canned response regardless of command.
// Used by the original hello/dir tests where command differentiation is not needed.

class qry_fake : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
} obj;

boost::property_tree::ptree qry_fake::netClient(const std::string &cmd, const std::string &arg) {
  boost::property_tree::ptree retval;

  retval.put("db.message", cmd);
  if (!arg.empty()) retval.put("db.argument", arg);
  retval.put("db.id", "core0");

  retval.put("db.stream.core0", "core0");
  retval.put("db.stream.core0.duration", "1");
  retval.put("db.stream.core0.size", "123");
  retval.put("db.stream.core0.count", "345");
  retval.put("db.stream.core0.location", "/dev/location");
  retval.put("db.stream.core0.cap", "789");

  retval.put("db.field.rname1", "rname1");
  retval.put("db.field.rname2", "rname2");
  retval.put("db", "world");
  return retval;
}

// Verify hello handshake succeeds when server responds with "world"
TEST(xqry, test_hello) {
  // covers: qry.qry.11~1
  EXPECT_TRUE(obj.hello() == boost::system::errc::success);
}

// Verify dir() formats single-stream table in the shared xqry form: header row, separator,
// columns left-aligned and joined by " | ", no edge pipes
TEST(xqry, test_dir) {
  // covers: qry.qry.12~1
  EXPECT_EQ(obj.dir().value(),
            "name  | duration | size | count | location      | cap\n"
            "------+----------+------+-------+---------------+----\n"
            "core0 | 1        | 123  | 345   | /dev/location | 789\n");
}

// Command-aware fake - dispatches different responses based on netClient command.
// Supports: hello, get, detail, adhoc, show. Used for detailShow/detailShowYaml, adhoc, and dirYaml tests.

class qry_fake_detail : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
};

boost::property_tree::ptree qry_fake_detail::netClient(const std::string &cmd, const std::string &arg) {
  boost::property_tree::ptree retval;

  if (cmd == "hello") {
    retval.put("db", "world");
  } else if (cmd == "get") {
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    retval.put("db.stream.core0.size", "123");
    retval.put("db.stream.core0.count", "345");
    retval.put("db.stream.core0.location", "/dev/location");
    retval.put("db.stream.core0.cap", "789");
  } else if (cmd == "detail") {
    retval.put("db.stream", arg);
    retval.put("db.duration", "1");
    retval.put("db.processed_line", "SELECT * STREAM " + arg + " FROM source");
    retval.put("db.field.rname1", "rname1");
    retval.put("db.field.rname2", "rname2");
    retval.put("db.field_type.rname1", "INTEGER");
    retval.put("db.field_type.rname2", "FLOAT");
  } else if (cmd == "adhoc") {
    retval.put("db", "OK");
  } else if (cmd == "show") {
    retval.put("db.stream", arg);
  }

  return retval;
}

// Verify detailShowYaml() produces correct YAML for an existing stream:
// checks stream name, delta, query line, field names, field types, and apiVersion header
TEST(xqry, test_detailShowYaml_found) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.detailShowYaml("core0").value();

  EXPECT_TRUE(result.find("name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
  EXPECT_TRUE(result.find("query: SELECT * STREAM core0 FROM source") != std::string::npos);
  EXPECT_TRUE(result.find("core0.rname1:") != std::string::npos);
  EXPECT_TRUE(result.find("core0.rname2:") != std::string::npos);
  EXPECT_TRUE(result.find("type: INTEGER") != std::string::npos);
  EXPECT_TRUE(result.find("type: FLOAT") != std::string::npos);
  EXPECT_TRUE(result.find("apiVersion: xqry/v1") != std::string::npos);
}

// Verify detailShow() prints the stream header and the field list as two column tables,
// in the same form as dir() and `xqry --bus`
TEST(xqry, test_detailShow_found) {
  // covers: qry.qry.13~1
  qry_fake_detail obj_detail;
  EXPECT_EQ(obj_detail.detailShow("core0").value(),
            "name  | delta | query\n"
            "------+-------+----------------------------------\n"
            "core0 | 1     | SELECT * STREAM core0 FROM source\n"
            "\n"
            "field        | type\n"
            "-------------+--------\n"
            "core0.rname1 | INTEGER\n"
            "core0.rname2 | FLOAT\n");
}

// Verify both detail forms return an empty string when the queried stream does not exist -
// the empty result is what makes the launcher report "no such stream"
TEST(xqry, test_detailShow_not_found) {
  // covers: qry.qry.13~1
  qry_fake_detail obj_detail;

  const auto detail = obj_detail.detailShow("nonexistent");
  ASSERT_FALSE(detail.has_value());
  EXPECT_EQ(detail.error(), selectResult::streamNotFound) << "literowka w nazwie to inna diagnoza niz cichy serwer";
  EXPECT_EQ(obj_detail.detailShowYaml("nonexistent").error(), selectResult::streamNotFound);
}

// Verify adhoc() returns false (success) when server acknowledges with "OK"
TEST(xqry, test_adhoc_success) {
  // covers: qry.qry.9~1
  qry_fake_detail obj_detail;
  auto result = obj_detail.adhoc("SELECT * STREAM test FROM core0");

  EXPECT_FALSE(result);
}

// Fake that always returns "FAIL" - simulates server rejecting an adhoc query

class qry_fake_adhoc_fail : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
};

boost::property_tree::ptree qry_fake_adhoc_fail::netClient(const std::string &cmd, const std::string &arg) {
  (void)cmd;
  (void)arg;
  boost::property_tree::ptree retval;
  retval.put("db", "FAIL");
  return retval;
}

// Verify adhoc() returns true (error) when server responds with non-"OK" value
TEST(xqry, test_adhoc_failure) {
  // covers: qry.qry.9~1
  qry_fake_adhoc_fail obj_fail;
  auto result = obj_fail.adhoc("INVALID QUERY");

  EXPECT_TRUE(result);
}

// Fake that responds with unexpected value instead of "world" - simulates protocol mismatch

class qry_fake_hello_fail : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
};

boost::property_tree::ptree qry_fake_hello_fail::netClient(const std::string &cmd, const std::string &arg) {
  boost::property_tree::ptree retval;
  retval.put("db", "unexpected_response");
  return retval;
}

// Verify hello() returns protocol_error when server responds with unexpected value
TEST(xqry, test_hello_failure) {
  // covers: qry.qry.11~1
  qry_fake_hello_fail obj_fail;
  EXPECT_TRUE(obj_fail.hello() == boost::system::errc::protocol_error);
}

class qry_fake_hello_empty : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
};

boost::property_tree::ptree qry_fake_hello_empty::netClient(const std::string &cmd, const std::string &arg) { return {}; }

// Verify hello() returns protocol_error when server responds with an empty payload
TEST(xqry, test_hello_empty_response) {
  // covers: qry.qry.11~1
  qry_fake_hello_empty obj_empty;
  EXPECT_TRUE(obj_empty.hello() == boost::system::errc::protocol_error);
}

// Verify dirYaml() produces correct YAML with apiVersion header, stream list,
// and all stream properties (name, delta, size, count, location)
TEST(xqry, test_dirYaml) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.dirYaml().value();

  EXPECT_TRUE(result.find("apiVersion: xqry/v1") != std::string::npos);
  EXPECT_TRUE(result.find("streams:") != std::string::npos);
  EXPECT_TRUE(result.find("- name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
  EXPECT_TRUE(result.find("size: 123") != std::string::npos);
  EXPECT_TRUE(result.find("count: 345") != std::string::npos);
  EXPECT_TRUE(result.find("location: /dev/location") != std::string::npos);
}

// Verify dirYaml() exact output for a deterministic single-stream response
TEST(xqry, test_dirYaml_exact_output) {
  // covers: qry.qry.12~1
  qry_fake_detail obj_detail;
  EXPECT_EQ(obj_detail.dirYaml().value(),
            "---\n"
            "apiVersion: xqry/v1\n"
            "streams:\n"
            "  - name: core0\n"
            "    delta: 1\n"
            "    size: 123\n"
            "    count: 345\n"
            "    location: /dev/location\n");
}

// Multi-stream fake - returns two streams (core0, core1) with different deltas and sizes.
// Used to verify dir/dirYaml/detailShow work correctly with multiple streams.

class qry_fake_multi : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
};

boost::property_tree::ptree qry_fake_multi::netClient(const std::string &cmd, const std::string &arg) {
  boost::property_tree::ptree retval;

  if (cmd == "hello") {
    retval.put("db", "world");
  } else if (cmd == "get") {
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    retval.put("db.stream.core0.size", "100");
    retval.put("db.stream.core0.count", "200");
    retval.put("db.stream.core0.location", "/dev/loc0");
    retval.put("db.stream.core0.cap", "300");

    retval.put("db.stream.core1", "core1");
    retval.put("db.stream.core1.duration", "0.5");
    retval.put("db.stream.core1.size", "400");
    retval.put("db.stream.core1.count", "500");
    retval.put("db.stream.core1.location", "/dev/loc1");
    retval.put("db.stream.core1.cap", "600");
  } else if (cmd == "detail") {
    retval.put("db.stream", arg);
    retval.put("db.duration", arg == "core0" ? "1" : "0.5");
    retval.put("db.processed_line", "DECLARE a INTEGER STREAM " + arg);
    retval.put("db.field.a", "a");
    retval.put("db.field_type.a", "INTEGER");
  }

  return retval;
}

// Verify dir() output contains both streams and their respective sizes
TEST(xqry, test_dir_multi_stream) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.dir().value();

  EXPECT_TRUE(result.find("core0") != std::string::npos);
  EXPECT_TRUE(result.find("core1") != std::string::npos);
  EXPECT_TRUE(result.find("100") != std::string::npos);
  EXPECT_TRUE(result.find("400") != std::string::npos);
}

// Verify dir() keeps column widths aligned across multi-stream output
TEST(xqry, test_dir_multi_stream_exact_output) {
  qry_fake_multi obj_multi;
  EXPECT_EQ(obj_multi.dir().value(),
            "name  | duration | size | count | location  | cap\n"
            "------+----------+------+-------+-----------+----\n"
            "core0 | 1        | 100  | 200   | /dev/loc0 | 300\n"
            "core1 | 0.5      | 400  | 500   | /dev/loc1 | 600\n");
}

// Verify dirYaml() lists both streams with correct names and deltas
TEST(xqry, test_dirYaml_multi_stream) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.dirYaml().value();

  EXPECT_TRUE(result.find("- name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("- name: core1") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 0.5") != std::string::npos);
}

// Verify detailShowYaml() resolves correct stream (core0) from a multi-stream setup
TEST(xqry, test_detailShowYaml_multi_stream_core0) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.detailShowYaml("core0").value();

  EXPECT_TRUE(result.find("name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
}

// Verify detailShowYaml() resolves correct stream (core1) with its own delta from a multi-stream setup
TEST(xqry, test_detailShowYaml_multi_stream_core1) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.detailShowYaml("core1").value();

  EXPECT_TRUE(result.find("name: core1") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 0.5") != std::string::npos);
}

// Verify detailShowYaml() exact YAML for deterministic single-stream metadata
TEST(xqry, test_detailShowYaml_exact_output) {
  // covers: qry.qry.13~1
  qry_fake_detail obj_detail;
  EXPECT_EQ(obj_detail.detailShowYaml("core0").value(),
            "---\n"
            "apiVersion: xqry/v1\n"
            "stream:\n"
            "  name: core0\n"
            "  delta: 1\n"
            "query: SELECT * STREAM core0 FROM source\n"
            "fields:\n"
            "  core0.rname1:\n"
            "    type: INTEGER\n"
            "  core0.rname2:\n"
            "    type: FLOAT\n");
}

// Fake with size="-1" and empty location - tests edge-case omission logic in dirYaml()

class qry_fake_nosize : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &cmd, const std::string &arg) override;
};

boost::property_tree::ptree qry_fake_nosize::netClient(const std::string &cmd, const std::string &arg) {
  (void)arg;
  boost::property_tree::ptree retval;

  if (cmd == "get") {
    retval.put("db.stream.str0", "str0");
    retval.put("db.stream.str0.duration", "2");
    retval.put("db.stream.str0.size", "-1");
    retval.put("db.stream.str0.count", "10");
    retval.put("db.stream.str0.location", "");
    retval.put("db.stream.str0.cap", "0");
  }

  return retval;
}

// Verify dirYaml() omits "size:" line when stream size is "-1" (unbounded)
TEST(xqry, test_dirYaml_size_minus_one_omitted) {
  // covers: qry.qry.12~1
  qry_fake_nosize obj_nosize;
  auto result = obj_nosize.dirYaml().value();

  EXPECT_TRUE(result.find("- name: str0") != std::string::npos);
  EXPECT_TRUE(result.find("size:") == std::string::npos);
}

// Verify dirYaml() omits "location:" line when stream location is empty
TEST(xqry, test_dirYaml_empty_location_omitted) {
  // covers: qry.qry.12~1
  qry_fake_nosize obj_nosize;
  auto result = obj_nosize.dirYaml().value();

  EXPECT_TRUE(result.find("location:") == std::string::npos);
}

// Verify default outputFormatMode is RAW
TEST(xqry, test_default_format_mode) {
  qry_fake_detail obj_detail;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::RAW);
}

// Verify all formatMode enum values can be assigned and read back
TEST(xqry, test_format_mode_set) {
  qry_fake_detail obj_detail;
  obj_detail.outputFormatMode = formatMode::GRAPHITE;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::GRAPHITE);

  obj_detail.outputFormatMode = formatMode::INFLUXDB;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::INFLUXDB);

  obj_detail.outputFormatMode = formatMode::GNUPLOT;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::GNUPLOT);
}

// === Regresje defektu klienta wykrytego w kampanii K6b (issue_215) ===
//
// Wspólny mianownik trzech trybów awarii: klient, który nie przeczytał ani
// jednego elementu, kończył się kodem 0 albo komunikatem o zupełnie innym
// błędzie. Dla harnessu i dla CI wyglądało to jak poprawny przebieg.
//
// Odtworzone tu z pomiarów na workerze: `xqry -s mon_000 -r` przy serwerze
// obciążonym rodziną W8 (Q=32) ginął po ~3 s przy serwerze pracującym 12 s.

// Serwer nie zdążył odpowiedzieć: `netClient` po wyczerpaniu prób zwraca ptree
// z samym `error.response` (ipcClient.cpp, gałąź "server not found").
class qry_fake_no_response : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    retval.put("error.response", "server not found");
    return retval;
  }
};

// Serwer odpowiedział, ale odpowiedź nie niesie listy strumieni.
class qry_fake_malformed : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    retval.put("db.message", "get");
    return retval;
  }
};

// Serwer odpowiedział poprawnie, ale nie zna żądanego strumienia.
class qry_fake_other_stream : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    retval.put("db.stream.core0.size", "123");
    retval.put("db.stream.core0.count", "345");
    retval.put("db.stream.core0.location", "/dev/location");
    retval.put("db.stream.core0.cap", "789");
    return retval;
  }
};

// Serwer zna strumień, więc `select()` wchodzi w pętlę i uruchamia producenta.
// W teście jednostkowym nie ma serwera IPC, więc kolejka odpowiedzi nigdy nie
// powstanie - to jest dokładnie tryb, który wcześniej dawał cichy sukces.
class qry_fake_known_stream : public qry {
 public:
  explicit qry_fake_known_stream(int responseQueueOpenMaxFails)
      : qry(kDefaultServerNoDataTimeoutMs, kDefaultClientResponseMaxFails, responseQueueOpenMaxFails) {}
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    retval.put("db.stream.core0.size", "123");
    retval.put("db.stream.core0.count", "345");
    retval.put("db.stream.core0.location", "/dev/location");
    retval.put("db.stream.core0.cap", "789");
    return retval;
  }
};

// B: brak odpowiedzi serwera musi być własnym trybem, a nie wyjątkiem
// „No such node (db.stream)". Stara wersja rzucała z `get_child`, launcher
// łapał to jako std::exception i zwracał `interrupted` - operator dostawał
// informację o przerwaniu zamiast o przeciążonym serwerze.
TEST(xqry, select_reports_server_no_response_instead_of_throwing) {
  // covers: qry.qry.4~1
  qry_fake_no_response obj_no_response;
  boost::program_options::variables_map vm;
  EXPECT_NO_THROW({
    const selectResult result = obj_no_response.select(vm, 0, "core0", {0, 0, 0});
    EXPECT_EQ(result, selectResult::serverNoResponse);
  });
}

// B: odpowiedź bez listy strumieni jest tym samym trybem - serwer nie dostarczył
// tego, o co pytano.
TEST(xqry, select_reports_server_no_response_on_malformed_answer) {
  // covers: qry.qry.2~1
  qry_fake_malformed obj_malformed;
  boost::program_options::variables_map vm;
  EXPECT_NO_THROW({
    const selectResult result = obj_malformed.select(vm, 0, "core0", {0, 0, 0});
    EXPECT_EQ(result, selectResult::serverNoResponse);
  });
}

// Nieznany strumień musi pozostać odróżnialny od przeciążonego serwera: to inna
// diagnoza (literówka w nazwie) i inna naprawa.
TEST(xqry, select_reports_stream_not_found_separately) {
  qry_fake_other_stream obj_other;
  boost::program_options::variables_map vm;
  const selectResult result = obj_other.select(vm, 0, "nieistniejacy", {0, 0, 0});
  EXPECT_EQ(result, selectResult::streamNotFound);
}

// A: sedno defektu. Kolejka odpowiedzi klienta nie powstaje, więc nie przychodzi
// ani jeden element. Wcześniej `select()` zwracał `true` i klient kończył się
// ZEREM; teraz musi zameldować, że kolejki nie było.
TEST(xqry, select_does_not_report_success_when_no_element_was_read) {
  // covers: qry.qry.8~1
  qry_fake_known_stream obj_known(2);  // dwie proby otwarcia, zeby test byl szybki
  boost::program_options::variables_map vm;
  // Przebieg z zadeklarowanym budzetem elementow (-m N), a nie nieograniczony: tylko taki
  // jest odporny na klawisz i tylko w takim producent dostaje swoje proby otwarcia kolejki.
  // Przebieg nieograniczony wiazal wynik testu ze stanem stdin: pod terminalem (CI daje
  // krokom pty) `_kbhit()` konczyl petle w pierwszym obrocie, producent byl zrywany, zanim
  // raz sprobowal, i select() slusznie meldowal `noData` -- bo o serwerze nie wiedzial nic.
  // Lokalnie, bez terminala, ta sama sciezka konczyla sie `clientQueueMissing` i test
  // przechodzil. Odtworzenie: `script -qec "test_xqry --gtest_filter=..." /dev/null`.
  const selectResult result = obj_known.select(vm, 1, "core0", {0, 0, 0});
  EXPECT_NE(result, selectResult::ok) << "klient bez ani jednego przeczytanego elementu nie moze konczyc sie sukcesem";
  EXPECT_EQ(result, selectResult::clientQueueMissing);
}

TEST(xqry, end_of_stream_before_the_first_row_reports_no_data) {
  // covers: qry.qry.8~1
  namespace IPC   = boost::interprocess;
  const auto name = ipc::names().responseQueue(getpid());
  struct RemoveQueue {
    std::string name;
    ~RemoveQueue() { IPC::message_queue::remove(name.c_str()); }
  } cleanup{name};
  IPC::message_queue::remove(name.c_str());
  IPC::message_queue queue(IPC::create_only, name.c_str(), 1, ipc::kResponseQueueMaxMessageSize,
                           IPC::permissions(ipc::kObjectPermissions));
  boost::property_tree::ptree end;
  end.put("stream", constants::Reserved_id_oob);
  std::ostringstream serialized;
  boost::property_tree::write_info(serialized, end);
  const auto message = serialized.str();
  queue.send(message.data(), message.size(), 0);
  qry_fake_known_stream client(2);
  boost::program_options::variables_map options;
  testing::internal::CaptureStdout();
  const auto result = client.select(options, 1, "core0", {0, 0, 0});
  const auto output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(result, selectResult::noData);
  EXPECT_TRUE(output.empty());
}

// Każdy tryb ma własny, niepusty opis - komunikat operatora nie może być pusty
// ani wspólny dla różnych awarii.
TEST(xqry, select_result_descriptions_are_distinct) {
  // covers: qry.to-string.2~1
  const std::array<selectResult, 7> all{
      selectResult::ok,     selectResult::streamNotFound, selectResult::serverNoResponse, selectResult::clientQueueMissing,
      selectResult::noData, selectResult::noActivePlan,   selectResult::serverStopping};
  std::set<std::string> seen;
  for (const auto result : all) {
    const std::string text = toString(result);
    EXPECT_FALSE(text.empty());
    seen.insert(text);
  }
  EXPECT_EQ(seen.size(), all.size()) << "tryby awarii musza byc rozroznialne w komunikacie";
  EXPECT_STREQ(toString(static_cast<selectResult>(255)), "unknown");
  EXPECT_STREQ(toString(selectResult::renderFailed), "select loop failed in the client; reason in the client log");
}

// === Weryfikacja zgloszenia [P2]: `xqry -d` maskuje brak odpowiedzi jako stan bezczynny ===

// Instancja bezczynna ODPOWIADA: handler wpisuje jedna wartosc pod kluczem `db`.
class qry_fake_idle : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    retval.put("db", "no active plan");
    return retval;
  }
};

// Serwer przyjal komende w trakcie wlasnego zamykania (executorsm: iLoopLimitCnt ==
// stop_now). Odpowiedzial - wiec meldowanie timeoutu byloby klamstwem o przyczynie.
class qry_fake_stopping : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    retval.put("db", "server stopping");
    return retval;
  }
};

TEST(xqry, dir_reports_no_response_instead_of_idle) {
  // covers: qry.qry.12~1
  qry_fake_no_response obj_no_response;
  const auto listing = obj_no_response.dir();
  ASSERT_FALSE(listing.has_value()) << "brak odpowiedzi serwera nie moze wygladac jak stan bezczynny";
  EXPECT_EQ(listing.error(), selectResult::serverNoResponse);
}

TEST(xqry, dirYaml_reports_no_response_instead_of_empty_list) {
  qry_fake_no_response obj_no_response;
  const auto listing = obj_no_response.dirYaml();
  ASSERT_FALSE(listing.has_value()) << "brak odpowiedzi serwera nie moze wygladac jak pusta lista";
  EXPECT_EQ(listing.error(), selectResult::serverNoResponse);
}

TEST(xqry, dir_still_names_idle_instance) {
  // covers: qry.qry.2~1
  qry_fake_idle obj_idle;
  EXPECT_EQ(obj_idle.dir().value(), "no active plan\n");
}

TEST(xqry, dirYaml_still_gives_empty_list_for_idle_instance) {
  // covers: qry.qry.12~1
  qry_fake_idle obj_idle;
  EXPECT_EQ(obj_idle.dirYaml().value(), "---\napiVersion: xqry/v1\nstreams: []\n");
}

// Zamykajacy sie serwer ma wlasny werdykt: inny komunikat i inny kod wyjscia niz milczenie.
TEST(xqry, dir_names_stopping_server_separately) {
  // covers: qry.qry.2~1, qry.qry.12~1
  qry_fake_stopping obj_stopping;
  const auto listing = obj_stopping.dir();
  ASSERT_FALSE(listing.has_value());
  EXPECT_EQ(listing.error(), selectResult::serverStopping);
}

TEST(xqry, select_names_stopping_server_separately) {
  // covers: qry.qry.4~1
  qry_fake_stopping obj_stopping;
  boost::program_options::variables_map vm;
  EXPECT_EQ(obj_stopping.select(vm, 0, "core0", {0, 0, 0}), selectResult::serverStopping);
}

TEST(xqry, detailShow_does_not_throw_for_idle_instance) {
  // covers: qry.qry.13~1
  qry_fake_idle obj_idle;
  EXPECT_NO_THROW({ EXPECT_EQ(obj_idle.detailShow("core0").error(), selectResult::noActivePlan); });
  EXPECT_NO_THROW({ EXPECT_EQ(obj_idle.detailShowYaml("core0").error(), selectResult::noActivePlan); });
}

TEST(xqry, detailShow_does_not_throw_when_server_is_silent) {
  // covers: qry.qry.13~1
  qry_fake_no_response obj_no_response;
  EXPECT_NO_THROW({ EXPECT_EQ(obj_no_response.detailShow("core0").error(), selectResult::serverNoResponse); });
}

// Plan przeladowany MIEDZY 'get' a 'detail': pierwsza odpowiedz niesie liste strumieni,
// druga juz tylko werdykt instancji bez planu. Przedtem `db.field` rzucalo tu wyjatkiem.
class qry_fake_plan_swapped_midway : public qry {
  bool listDelivered_{false};

 public:
  boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/) override {
    boost::property_tree::ptree retval;
    if (listDelivered_) {
      retval.put("db", "no active plan");
      return retval;
    }
    listDelivered_ = true;
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    return retval;
  }
};

TEST(xqry, detailShow_survives_plan_swap_between_get_and_detail) {
  // covers: qry.qry.13~1
  qry_fake_plan_swapped_midway obj_swapped;
  EXPECT_NO_THROW({ EXPECT_EQ(obj_swapped.detailShow("core0").error(), selectResult::noActivePlan); });
}

namespace {
class RecordingQry : public qry {
 public:
  std::vector<std::pair<std::string, std::string>> calls;
  std::vector<boost::property_tree::ptree> replies;
  boost::property_tree::ptree netClient(const std::string &command, const std::string &argument) override {
    calls.emplace_back(command, argument);
    if (calls.size() <= replies.size()) return replies[calls.size() - 1];
    boost::property_tree::ptree reply;
    reply.put("db", "OK");
    return reply;
  }
};
}  // namespace

TEST(xqry, reset_preserves_binary_text_across_chunks_and_commits_last) {
  // covers: qry.qry.10~1
  RecordingQry client;
  std::string plan(805, 'x');
  plan.replace(398, 7, std::string("\n'\\\"\0\r\n", 7));
  EXPECT_FALSE(client.reset(plan));
  ASSERT_EQ(client.calls.size(), 5U);
  EXPECT_EQ(client.calls.front(), (std::pair<std::string, std::string>{"reset-begin", "3"}));
  std::string restored;
  for (std::size_t index = 1; index <= 3; ++index) {
    EXPECT_EQ(client.calls[index].first, "reset-chunk");
    EXPECT_EQ(client.calls[index].second.size(), index == 3 ? 5U : 400U);
    restored += client.calls[index].second;
  }
  EXPECT_EQ(restored, plan);
  EXPECT_EQ(client.calls.back(), (std::pair<std::string, std::string>{"reset-commit", ""}));
}

TEST(xqry, empty_reset_has_zero_chunks_and_empty_adhoc_sends_nothing) {
  // covers: qry.qry.9~1, qry.qry.10~1
  RecordingQry client;
  EXPECT_TRUE(client.adhoc(""));
  EXPECT_TRUE(client.calls.empty());
  EXPECT_FALSE(client.reset(""));
  EXPECT_EQ(client.calls, (std::vector<std::pair<std::string, std::string>>{{"reset-begin", "0"}, {"reset-commit", ""}}));
}

TEST(xqry, reset_stops_at_each_refusal_and_prints_the_stage_once) {
  // covers: qry.qry.10~1
  for (std::size_t failed = 0; failed < 5; ++failed) {
    SCOPED_TRACE(failed);
    RecordingQry client;
    client.replies.resize(failed + 1);
    for (auto &reply : client.replies)
      reply.put("db", "OK");
    client.replies.back().put("db", "denied");
    testing::internal::CaptureStderr();
    const bool refused = client.reset(std::string(805, 'x'));
    const auto error   = testing::internal::GetCapturedStderr();
    EXPECT_TRUE(refused);
    ASSERT_EQ(client.calls.size(), failed + 1);
    EXPECT_EQ(error, "xqry: plan reload refused at " + client.calls.back().first + ": denied\n");
  }
}

TEST(xqry, response_error_precedes_streams_and_streams_precede_idle_marker) {
  // covers: qry.qry.2~1
  for (const bool hasError : {false, true}) {
    RecordingQry client;
    boost::property_tree::ptree answer;
    answer.put("db", "no active plan");
    answer.put("db.stream.core0", "core0");
    answer.put("db.stream.core0.duration", "1");
    answer.put("db.stream.core0.size", "-1");
    answer.put("db.stream.core0.count", "0");
    answer.put("db.stream.core0.cap", "0");
    answer.put("db.stream.core0.location", "");
    if (hasError) answer.put("error.response", "timeout");
    client.replies.push_back(answer);
    const auto result = client.dirYaml();
    ASSERT_EQ(result.has_value(), !hasError);
    if (hasError)
      EXPECT_EQ(result.error(), selectResult::serverNoResponse);
    else
      EXPECT_TRUE(result->contains("- name: core0"));
  }
}

TEST(xqry, select_subscribes_first_and_preserves_the_get_verdict_after_show_refusal) {
  // covers: qry.qry.3~1, qry.qry.4~1
  for (const bool known : {false, true}) {
    RecordingQry client;
    boost::property_tree::ptree show, get;
    show.put("error.response", "subscription refused");
    get.put("db.stream.other", "other");
    if (known) get.put("db.stream.core0", "core0");
    client.replies = {show, get};
    boost::program_options::variables_map vm;
    EXPECT_EQ(client.select(vm, 1, "core0", {0, 0, 0}), known ? selectResult::clientQueueMissing : selectResult::streamNotFound);
    EXPECT_EQ(client.calls, (std::vector<std::pair<std::string, std::string>>{{"show", "core0"}, {"get", ""}}));
  }
}

TEST(xqry, result_descriptions_match_the_public_diagnostics) {
  // covers: qry.to-string.1~1
  const std::array<const char *, 8> expected{"ok",
                                             "stream unknown to the server",
                                             "server did not answer within the timeout",
                                             "server did not create the client response queue",
                                             "no data in stream",
                                             "server has no plan loaded (idle); load one with --reset",
                                             "server is shutting down",
                                             "select loop failed in the client; reason in the client log"};
  for (std::size_t index = 0; index < expected.size(); ++index)
    EXPECT_STREQ(toString(static_cast<selectResult>(index)), expected[index]);
}

TEST(xqry, json_commands_emit_versioned_lines_and_errors) {
  // covers: qry.qry.14~1
  qry_fake_detail client;
  testing::internal::CaptureStdout();
  EXPECT_EQ(client.jsonCommand("hello", "", 0, 0), 0);
  const auto pong = boost::json::parse(testing::internal::GetCapturedStdout()).as_object();
  EXPECT_EQ(pong.at("version").as_int64(), 1);
  EXPECT_EQ(pong.at("event").as_string(), "pong");
  testing::internal::CaptureStdout();
  EXPECT_EQ(client.jsonCommand("dir", "", 0, 0), 0);
  const auto streams = boost::json::parse(testing::internal::GetCapturedStdout()).as_object();
  EXPECT_EQ(streams.at("event").as_string(), "streams");
  ASSERT_EQ(streams.at("streams").as_array().size(), 1U);
  EXPECT_EQ(streams.at("streams").as_array()[0].as_object().at("name").as_string(), "core0");

  qry_fake_idle idle;
  testing::internal::CaptureStdout();
  EXPECT_EQ(idle.jsonCommand("detail", "core0", 0, 0), 1);
  const auto error = boost::json::parse(testing::internal::GetCapturedStdout()).as_object();
  EXPECT_EQ(error.at("version").as_int64(), 1);
  EXPECT_EQ(error.at("event").as_string(), "error");
  EXPECT_EQ(error.at("code").as_string(), "no_active_plan");
}

TEST(xqry, detail_forms_preserve_stopping_and_plan_swap_verdicts) {
  // covers: qry.qry.13~1
  for (const bool yaml : {false, true}) {
    qry_fake_stopping stopping;
    const auto stopped = yaml ? stopping.detailShowYaml("core0") : stopping.detailShow("core0");
    ASSERT_FALSE(stopped.has_value());
    EXPECT_EQ(stopped.error(), selectResult::serverStopping);
    qry_fake_plan_swapped_midway swapped;
    const auto reloaded = yaml ? swapped.detailShowYaml("core0") : swapped.detailShow("core0");
    ASSERT_FALSE(reloaded.has_value());
    EXPECT_EQ(reloaded.error(), selectResult::noActivePlan);
  }
}
