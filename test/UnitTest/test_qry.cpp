#include <gtest/gtest.h>

#include <array>
#include <set>
#include <string>

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
TEST(xqry, test_hello) { EXPECT_TRUE(obj.hello() == boost::system::errc::success); }

// Verify dir() formats single-stream table in the shared xqry form: header row, separator,
// columns left-aligned and joined by " | ", no edge pipes
TEST(xqry, test_dir) {
  EXPECT_EQ(obj.dir(),
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
  auto result = obj_detail.detailShowYaml("core0");

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
  qry_fake_detail obj_detail;
  EXPECT_EQ(obj_detail.detailShow("core0"),
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
  qry_fake_detail obj_detail;

  EXPECT_TRUE(obj_detail.detailShow("nonexistent").empty());
  EXPECT_TRUE(obj_detail.detailShowYaml("nonexistent").empty());
}

// Verify adhoc() returns false (success) when server acknowledges with "OK"
TEST(xqry, test_adhoc_success) {
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
  qry_fake_hello_empty obj_empty;
  EXPECT_TRUE(obj_empty.hello() == boost::system::errc::protocol_error);
}

// Verify dirYaml() produces correct YAML with apiVersion header, stream list,
// and all stream properties (name, delta, size, count, location)
TEST(xqry, test_dirYaml) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.dirYaml();

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
  qry_fake_detail obj_detail;
  EXPECT_EQ(obj_detail.dirYaml(),
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
  auto result = obj_multi.dir();

  EXPECT_TRUE(result.find("core0") != std::string::npos);
  EXPECT_TRUE(result.find("core1") != std::string::npos);
  EXPECT_TRUE(result.find("100") != std::string::npos);
  EXPECT_TRUE(result.find("400") != std::string::npos);
}

// Verify dir() keeps column widths aligned across multi-stream output
TEST(xqry, test_dir_multi_stream_exact_output) {
  qry_fake_multi obj_multi;
  EXPECT_EQ(obj_multi.dir(),
            "name  | duration | size | count | location  | cap\n"
            "------+----------+------+-------+-----------+----\n"
            "core0 | 1        | 100  | 200   | /dev/loc0 | 300\n"
            "core1 | 0.5      | 400  | 500   | /dev/loc1 | 600\n");
}

// Verify dirYaml() lists both streams with correct names and deltas
TEST(xqry, test_dirYaml_multi_stream) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.dirYaml();

  EXPECT_TRUE(result.find("- name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("- name: core1") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 0.5") != std::string::npos);
}

// Verify detailShowYaml() resolves correct stream (core0) from a multi-stream setup
TEST(xqry, test_detailShowYaml_multi_stream_core0) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.detailShowYaml("core0");

  EXPECT_TRUE(result.find("name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
}

// Verify detailShowYaml() resolves correct stream (core1) with its own delta from a multi-stream setup
TEST(xqry, test_detailShowYaml_multi_stream_core1) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.detailShowYaml("core1");

  EXPECT_TRUE(result.find("name: core1") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 0.5") != std::string::npos);
}

// Verify detailShowYaml() exact YAML for deterministic single-stream metadata
TEST(xqry, test_detailShowYaml_exact_output) {
  qry_fake_detail obj_detail;
  EXPECT_EQ(obj_detail.detailShowYaml("core0"),
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
  qry_fake_nosize obj_nosize;
  auto result = obj_nosize.dirYaml();

  EXPECT_TRUE(result.find("- name: str0") != std::string::npos);
  EXPECT_TRUE(result.find("size:") == std::string::npos);
}

// Verify dirYaml() omits "location:" line when stream location is empty
TEST(xqry, test_dirYaml_empty_location_omitted) {
  qry_fake_nosize obj_nosize;
  auto result = obj_nosize.dirYaml();

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
// powstanie — to jest dokładnie tryb, który wcześniej dawał cichy sukces.
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
// łapał to jako std::exception i zwracał `interrupted` — operator dostawał
// informację o przerwaniu zamiast o przeciążonym serwerze.
TEST(xqry, select_reports_server_no_response_instead_of_throwing) {
  qry_fake_no_response obj_no_response;
  boost::program_options::variables_map vm;
  EXPECT_NO_THROW({
    const selectResult result = obj_no_response.select(vm, 0, "core0", {0, 0, 0});
    EXPECT_EQ(result, selectResult::serverNoResponse);
  });
}

// B: odpowiedź bez listy strumieni jest tym samym trybem — serwer nie dostarczył
// tego, o co pytano.
TEST(xqry, select_reports_server_no_response_on_malformed_answer) {
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

// Każdy tryb ma własny, niepusty opis — komunikat operatora nie może być pusty
// ani wspólny dla różnych awarii.
TEST(xqry, select_result_descriptions_are_distinct) {
  const std::array<selectResult, 5> all{selectResult::ok, selectResult::streamNotFound, selectResult::serverNoResponse,
                                        selectResult::clientQueueMissing, selectResult::noData};
  std::set<std::string> seen;
  for (const auto result : all) {
    const std::string text = toString(result);
    EXPECT_FALSE(text.empty());
    seen.insert(text);
  }
  EXPECT_EQ(seen.size(), all.size()) << "tryby awarii musza byc rozroznialne w komunikacie";
}
