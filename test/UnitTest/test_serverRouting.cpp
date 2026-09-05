#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "qry/serverRouting.hpp"

namespace {

// Reguly routingu sa tu sprawdzane BEZ pamieci dzielonej i bez startowania serwerow:
// routing::* pracuje na gotowej migawce magistrali, wiec migawke da sie zbudowac recznie.
// Scenariusz wieloserwerowy w /dev/shm pokrywa it_multiserver_routing.

bus::InstanceInfo makeInstance(const std::string &name, std::int32_t pid, const std::string &queryFile,
                               const std::vector<std::string> &streams, std::uint32_t modes = 0) {
  bus::InstanceInfo retVal;
  retVal.name      = name;
  retVal.pid       = pid;
  retVal.queryFile = queryFile;
  retVal.modes     = modes;
  retVal.streams   = streams;
  return retVal;
}

std::vector<bus::InstanceInfo> twoInstances() {
  return {makeInstance("alfa", 101, "alfa.rql", {"srca", "dsta"}), makeInstance("beta", 202, "beta.rql", {"srcb", "dstb"})};
}

TEST(serverRouting, unnamedInstanceHasStableLabel) {
  EXPECT_EQ(routing::instanceLabel(""), "(unnamed)");
  EXPECT_EQ(routing::instanceLabel("alfa"), "alfa");
}

TEST(serverRouting, emptyBusMeansHistoricNames) {
  // Brak instancji => nazwa pusta, czyli nazwy historyczne. Zepsuta albo pusta magistrala
  // nie moze unieruchomic klienta.
  const routing::Resolution resolved = routing::forSingleTarget({});
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_TRUE(resolved.serverName.empty());
}

TEST(serverRouting, singleInstanceResolvesWithoutStreamCheck) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("alfa", 101, "alfa.rql", {"dsta"})};
  const routing::Resolution resolved = routing::forSingleTarget(instances);
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "alfa");
}

TEST(serverRouting, wholeInstanceCommandIsAmbiguousWithManyInstances) {
  const routing::Resolution resolved = routing::forSingleTarget(twoInstances());
  EXPECT_EQ(resolved.status, routing::Status::Ambiguous);
  EXPECT_NE(resolved.detail.find("alfa"), std::string::npos);
  EXPECT_NE(resolved.detail.find("beta"), std::string::npos);
  EXPECT_NE(resolved.detail.find("--server"), std::string::npos);
}

TEST(serverRouting, streamResolvesToItsOwner) {
  const std::vector<bus::InstanceInfo> instances = twoInstances();
  EXPECT_EQ(routing::forStream(instances, "dsta").serverName, "alfa");
  EXPECT_EQ(routing::forStream(instances, "dstb").serverName, "beta");
  EXPECT_EQ(routing::forStream(instances, "srcb").serverName, "beta");
}

TEST(serverRouting, unknownStreamIsNotFoundNotTimeout) {
  // Kod wyjscia tej sciezki musi zostac kodem "nie ma takiego strumienia", a nie "serwer
  // milczy" -- issue_215 rozdzielil te dwie diagnozy celowo.
  const routing::Resolution resolved = routing::forStream(twoInstances(), "nosuch");
  EXPECT_EQ(resolved.status, routing::Status::StreamNotFound);
  EXPECT_NE(resolved.detail.find("nosuch"), std::string::npos);
}

TEST(serverRouting, unnamedInstanceIsRoutableTarget) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("", 101, "query.rql", {"dst1"}),
                                                 makeInstance("beta", 202, "beta.rql", {"dstb"})};
  const routing::Resolution resolved = routing::forStream(instances, "dst1");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_TRUE(resolved.serverName.empty());
}

TEST(serverRouting, sourceStreamsIgnoreSelectIdentifiersAndQuotedLiterals) {
  const std::vector<std::string> found =
      routing::extractSourceStreams("SELECT to_string(dsta[0]:8)+'dstb' STREAM x FROM dsta FILE 'dstb'");
  EXPECT_EQ(found, (std::vector<std::string>{"dsta"}));
}

TEST(serverRouting, sourceStreamsFollowRqlIdGrammar) {
  const std::vector<std::string> found = routing::extractSourceStreams("SELECT cell$0[0] STREAM y FROM cell$0#str12");
  EXPECT_EQ(found, (std::vector<std::string>{"cell$0", "str12"}));
}

TEST(serverRouting, sourceStreamsIgnoreReducersAggregatorsAndComments) {
  const std::vector<std::string> found = routing::extractSourceStreams(
      "SELECT 1 STREAM y FROM MIN(dsta).avg/* dstb /* nested */ dstb */+srca // dstb\n RETENTION 4");
  EXPECT_EQ(found, (std::vector<std::string>{"dsta", "srca"}));
}

// RQL.g4 leksuje slowa kluczowe wielkosciowo (`MIN: 'MIN'|'min'`), wiec pisownia mieszana jest
// zwykla nazwa strumienia -- grammar mowi to wprost przy stream_fn_call. Lekser routingu ma sie
// trzymac tej samej reguly: `Min` i `From` to zrodla, `min` i `from` to skladnia.
TEST(serverRouting, sourceStreamsTreatMixedCaseKeywordsAsStreamNames) {
  EXPECT_EQ(routing::extractSourceStreams("SELECT Min[0] STREAM y FROM Min"), (std::vector<std::string>{"Min"}));
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y from Storage+MIN(a)"), (std::vector<std::string>{"Storage", "a"}));
  // Pisownie z grammar nadal sa skladnia: `min` to reduktor, `storage` konczy wyrazenie FROM.
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y FROM min(a) STORAGE 'memory'"), (std::vector<std::string>{"a"}));
}

TEST(serverRouting, adHocResolvesToTheOnlyOwner) {
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT dsta[0]+1 STREAM tmp FROM dsta");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "alfa");
}

TEST(serverRouting, adHocIgnoresFieldFunctionAndOutputNameCollisions) {
  const std::vector<bus::InstanceInfo> instances{
      makeInstance("alfa", 101, "alfa.rql", {"dsta"}),
      makeInstance("beta", 202, "beta.rql", {"to_string", "temperature", "tmp", "avg"})};
  const routing::Resolution resolved =
      routing::forAdHoc(instances, "SELECT to_string(dsta.temperature:8) STREAM tmp FROM dsta.avg");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "alfa");
}

TEST(serverRouting, adHocRoutesGeneratedDollarStream) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("alfa", 101, "alfa.rql", {"cell$0"}),
                                                 makeInstance("beta", 202, "beta.rql", {"cell"})};
  const routing::Resolution resolved = routing::forAdHoc(instances, "SELECT cell$0[0] STREAM tmp FROM cell$0");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "alfa");
}

TEST(serverRouting, adHocAcrossInstancesIsRejected) {
  // Rozglaszanie ad-hoc jest wykluczone: getAdHoc modyfikuje PLAN serwera, wiec trafienie
  // w niewlasciwa instancje to trwaly skutek uboczny, a nie pomylka do powtorzenia.
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT dsta[0]+dstb[0] STREAM tmp FROM dsta,dstb");
  EXPECT_EQ(resolved.status, routing::Status::CrossServer);
  EXPECT_EQ(resolved.detail, "ad-hoc query crosses a server boundary (dsta@alfa, dstb@beta)");
}

TEST(serverRouting, adHocWithoutKnownStreamIsUndecidable) {
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT 1 STREAM tmp FROM nowhere");
  EXPECT_EQ(resolved.status, routing::Status::Ambiguous);
  EXPECT_NE(resolved.detail.find("--server"), std::string::npos);
}

TEST(serverRouting, describeIsSortedAndCarriesStreams) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("beta", 202, "beta.rql", {"srcb", "dstb"}),
                                                 makeInstance("alfa", 101, "/home/rdb/plans/alfa.rql", {"srca", "dsta"})};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 5U);
  EXPECT_EQ(lines[0], "SERVER | PID | MODE | QUERY              | STREAMS");
  EXPECT_EQ(lines[1], "-------+-----+------+--------------------+-----------");
  EXPECT_EQ(lines[2], "alfa   | 101 | N    | .../plans/alfa.rql | srca, dsta");
  EXPECT_EQ(lines[3], "beta   | 202 | N    | beta.rql           | srcb, dstb");
  EXPECT_EQ(lines[4], "MODE: N=normal, R=realtime, F=no-clock, U=until-eof, M=llimitqry, X=xqrywait, S=service");
}

TEST(serverRouting, describeMarksMissingQueryFile) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("", 101, "", {"dst1"})};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 4U);
  EXPECT_EQ(lines[0], "SERVER    | PID | MODE | QUERY | STREAMS");
  EXPECT_EQ(lines[1], "----------+-----+------+-------+--------");
  EXPECT_EQ(lines[2], "(unnamed) | 101 | N    | -     | dst1");
}

TEST(serverRouting, describeWidensColumnsForGeneratedNames) {
  // Nazwa z --autoname jest dluzsza od naglowka SERVER, a tryby moga wystapic razem: obie
  // kolumny musza sie rozsunac, a nie rozjechac wiersz.
  const std::vector<bus::InstanceInfo> instances{
      makeInstance("nostalgic-ptolemy", 7, "plan.rql", {"dst1"}, bus::mode::kRealTime | bus::mode::kService),
      makeInstance("alfa", 101, "plan.rql", {"dst2"}, bus::mode::kNoClock)};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 5U);
  EXPECT_EQ(lines[0], "SERVER            | PID | MODE | QUERY    | STREAMS");
  EXPECT_EQ(lines[2], "alfa              | 101 | F    | plan.rql | dst2");
  EXPECT_EQ(lines[3], "nostalgic-ptolemy | 7   | RS   | plan.rql | dst1");
}

TEST(serverRouting, describeSpellsOutEveryRunMode) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("alfa", 1, "plan.rql", {"dst1"},
                                                              bus::mode::kRealTime | bus::mode::kNoClock | bus::mode::kUntilEof |
                                                                  bus::mode::kLoopLimit | bus::mode::kXqryWait |
                                                                  bus::mode::kService)};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 4U);
  EXPECT_EQ(lines[2], "alfa   | 1   | RFUMXS | plan.rql | dst1");
}

TEST(serverRouting, describeYamlKeepsOrderAndFullQueryPath) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("beta", 202, "beta.rql", {"srcb"}, bus::mode::kService),
                                                 makeInstance("alfa", 101, "/home/rdb/plans/alfa.rql", {"srca", "dsta"})};
  const std::vector<std::string> lines = routing::describeYaml(instances);
  ASSERT_EQ(lines.size(), 16U);
  EXPECT_EQ(lines[0], "---");
  EXPECT_EQ(lines[1], "apiVersion: xqry/v1");
  EXPECT_EQ(lines[2], "servers:");
  EXPECT_EQ(lines[3], "  - name: alfa");
  EXPECT_EQ(lines[4], "    pid: 101");
  EXPECT_EQ(lines[5], "    modes: N");
  // Sciezka W CALOSCI, inaczej niz skrocone `.../plans/alfa.rql` w tabeli.
  EXPECT_EQ(lines[6], "    query: \"/home/rdb/plans/alfa.rql\"");
  EXPECT_EQ(lines[7], "    streams:");
  EXPECT_EQ(lines[8], "      - srca");
  EXPECT_EQ(lines[9], "      - dsta");
  EXPECT_EQ(lines[10], "  - name: beta");
  EXPECT_EQ(lines[12], "    modes: S");
}

TEST(serverRouting, describeYamlDescribesEmptyBusAndEmptyFields) {
  const std::vector<std::string> empty = routing::describeYaml({});
  ASSERT_EQ(empty.size(), 3U);
  EXPECT_EQ(empty[2], "servers: []");

  // Instancja bezimienna bez pliku zapytania i bez strumieni: klucz `query` znika (nie ma
  // czego podac), a pusta lista strumieni zostaje jawnym `[]`.
  const std::vector<std::string> lines = routing::describeYaml({makeInstance("", 7, "", {})});
  ASSERT_EQ(lines.size(), 7U);
  EXPECT_EQ(lines[3], "  - name: (unnamed)");
  EXPECT_EQ(lines[4], "    pid: 7");
  EXPECT_EQ(lines[5], "    modes: N");
  EXPECT_EQ(lines[6], "    streams: []");
}

}  // namespace
