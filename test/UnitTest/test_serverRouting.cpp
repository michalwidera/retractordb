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

TEST(serverRouting, adHocRuleResolvesByItsOnStream) {
  // RULE nie ma klauzuli FROM, wiec do 2026-09-05 bylo dla routingu nierozstrzygalne i bez
  // --server nie trafialo nigdzie. Adresat jest jednak jednoznaczny: wlasciciel strumienia
  // spod ON, bo tam regula zamieszka.
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "RULE watch ON dstb WHEN dstb[0] > 3 DO DUMP -2 TO 2");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "beta");
}

TEST(serverRouting, adHocRuleNameIsNotAStreamName) {
  // Nazwa reguly stoi w tym samym zapisie co nazwa strumienia i w dodatku PRZED nia. Gdyby
  // routing bral ja pod uwage, regula nazwana jak cudzy strumien jechalaby do cudzej instancji.
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "RULE dsta ON dstb WHEN dstb[0] > 3 DO DUMP -2 TO 2");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "beta");
}

TEST(serverRouting, adHocRuleOnUnknownStreamIsUndecidable) {
  const routing::Resolution resolved =
      routing::forAdHoc(twoInstances(), "RULE watch ON nowhere WHEN nowhere[0] > 3 DO DUMP -2 TO 2");
  EXPECT_EQ(resolved.status, routing::Status::Ambiguous);
  EXPECT_NE(resolved.detail.find("--server"), std::string::npos);
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

TEST(serverRouting, adHocDeclareRequiresExplicitTargetWithMultipleServers) {
  const routing::Resolution resolved =
      routing::forAdHoc(twoInstances(), "DECLARE value INTEGER STREAM late, 0.1 FILE 'late.txt'");
  EXPECT_EQ(resolved.status, routing::Status::Ambiguous);
  EXPECT_NE(resolved.detail.find("--server"), std::string::npos);
}

TEST(serverRouting, describeIsSortedAndCarriesStreams) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("beta", 202, "beta.rql", {"srcb", "dstb"}),
                                                 makeInstance("alfa", 101, "/home/rdb/plans/alfa.rql", {"srca", "dsta"})};
  const std::vector<std::string> lines = routing::describe(instances);
  // Instancja o dwoch strumieniach zajmuje dwie linie: kolejne nazwy ida w liniach
  // kontynuacji z pustymi kolumnami po lewej, wiec przynaleznosc nadal widac po kreskach.
  ASSERT_EQ(lines.size(), 7U);
  EXPECT_EQ(lines[0], "SERVER | PID | MODE | QUERY              | STREAMS");
  EXPECT_EQ(lines[1], "-------+-----+------+--------------------+--------");
  EXPECT_EQ(lines[2], "alfa   | 101 | N    | .../plans/alfa.rql | srca");
  EXPECT_EQ(lines[3], "       |     |      |                    | dsta");
  EXPECT_EQ(lines[4], "beta   | 202 | N    | beta.rql           | srcb");
  EXPECT_EQ(lines[5], "       |     |      |                    | dstb");
  EXPECT_EQ(lines[6], "MODE: N=normal, R=realtime, F=no-clock, U=until-eof, M=llimitqry, X=xqrywait, S=service");
}

TEST(serverRouting, describeMarksMissingQueryFile) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("", 101, "", {"dst1"})};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 4U);
  EXPECT_EQ(lines[0], "SERVER    | PID | MODE | QUERY | STREAMS");
  EXPECT_EQ(lines[1], "----------+-----+------+-------+--------");
  EXPECT_EQ(lines[2], "(unnamed) | 101 | N    | -     | dst1");
}

TEST(serverRouting, describeMarksInstanceWithoutStreams) {
  // Instancja bez ani jednego strumienia nie moze dac wiersza urwanego na kresce: kolumna
  // STREAMS dostaje myslnik, tak samo jak QUERY przy braku pliku zapytania.
  const std::vector<std::string> lines = routing::describe({makeInstance("alfa", 101, "plan.rql", {})});
  ASSERT_EQ(lines.size(), 4U);
  EXPECT_EQ(lines[2], "alfa   | 101 | N    | plan.rql | -");
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
  EXPECT_EQ(lines[1], "------------------+-----+------+----------+--------");
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

TEST(serverRouting, describeNamespacesYamlKeepsOrderAndFullQueryPath) {
  const std::vector<routing::NamespaceGroup> groups{
      {.name      = "",
       .instances = {makeInstance("beta", 202, "beta.rql", {"srcb"}, bus::mode::kService),
                     makeInstance("alfa", 101, "/home/rdb/plans/alfa.rql", {"srca", "dsta"})}}};
  const std::vector<std::string> lines = routing::describeNamespacesYaml(groups);
  ASSERT_EQ(lines.size(), 18U);
  EXPECT_EQ(lines[0], "---");
  EXPECT_EQ(lines[1], "apiVersion: xqry/v1");
  EXPECT_EQ(lines[2], "servers:");
  EXPECT_EQ(lines[3], "  - name: alfa");
  EXPECT_EQ(lines[4], "    namespace: null");
  EXPECT_EQ(lines[5], "    pid: 101");
  EXPECT_EQ(lines[6], "    modes: N");
  // Sciezka W CALOSCI, inaczej niz skrocone `.../plans/alfa.rql` w tabeli.
  EXPECT_EQ(lines[7], "    query: \"/home/rdb/plans/alfa.rql\"");
  EXPECT_EQ(lines[8], "    streams:");
  EXPECT_EQ(lines[9], "      - srca");
  EXPECT_EQ(lines[10], "      - dsta");
  EXPECT_EQ(lines[11], "  - name: beta");
  EXPECT_EQ(lines[14], "    modes: S");
}

TEST(serverRouting, describeNamespacesYamlDescribesEmptyBusAndEmptyFields) {
  const std::vector<std::string> empty = routing::describeNamespacesYaml({});
  ASSERT_EQ(empty.size(), 3U);
  EXPECT_EQ(empty[2], "servers: []");

  // Instancja bezimienna bez pliku zapytania i bez strumieni: klucz `query` znika (nie ma
  // czego podac), a pusta lista strumieni zostaje jawnym `[]`.
  const std::vector<std::string> lines =
      routing::describeNamespacesYaml({{.name = "", .instances = {makeInstance("", 7, "", {})}}});
  ASSERT_EQ(lines.size(), 8U);
  EXPECT_EQ(lines[3], "  - name: (unnamed)");
  EXPECT_EQ(lines[5], "    pid: 7");
  EXPECT_EQ(lines[6], "    modes: N");
  EXPECT_EQ(lines[7], "    streams: []");
}

TEST(serverRouting, describeNamespacesYamlDistinguishesDefaultFromNamedNull) {
  const std::vector<routing::NamespaceGroup> groups{
      {.name = "", .instances = {makeInstance("shared", 101, "default.rql", {})}},
      {.name = "null", .instances = {makeInstance("shared", 202, "named.rql", {})}}};
  const std::vector<std::string> lines = routing::describeNamespacesYaml(groups);
  ASSERT_EQ(lines.size(), 15U);
  EXPECT_EQ(lines[3], "  - name: shared");
  EXPECT_EQ(lines[4], "    namespace: null");
  EXPECT_EQ(lines[9], "  - name: shared");
  EXPECT_EQ(lines[10], "    namespace: \"null\"");
  EXPECT_EQ(routing::describeNamespacesYaml({})[2], "servers: []");
}

// Wymagania z serverRouting.hpp, ktorych dotad nic nie sprawdzalo wprost. Teksty zapytan sa
// wejsciem leksera routingu: tam, gdzie przypadek wymaga tekstu spoza RQL.g4, mowi to komentarz.

TEST(serverRouting, sourceStreamsSkipBackslashEscapeInsideLiteral) {
  // Bez ucieczki literal konczylby sie na `\'`, `FROM fake` dalby nazwe `fake`, a reszta tekstu
  // zostalaby literalem do konca. RQL.g4 (STRING) ucieczki odwrotnym ukosnikiem nie zna.
  EXPECT_EQ(routing::extractSourceStreams("SELECT 'x\\' FROM fake' STREAM y FROM a"), (std::vector<std::string>{"a"}));
}

TEST(serverRouting, sourceStreamsEndLineCommentAtNewlineAndKeepUnderscore) {
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y FROM a // b\n + c_d"), (std::vector<std::string>{"a", "c_d"}));
}

TEST(serverRouting, sourceStreamsTakeIdentifierRightAfterOn) {
  EXPECT_EQ(routing::extractSourceStreams("RULE dsta ON dstb WHEN dstb[0] > 3 DO DUMP -2 TO 2"),
            (std::vector<std::string>{"dstb"}));
  EXPECT_EQ(routing::extractSourceStreams("RULE r on a WHEN a[0] > 1 DO DUMP -1 TO 1"), (std::vector<std::string>{"a"}));
}

TEST(serverRouting, sourceStreamsSkipEveryReducerAndNameAfterDot) {
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y FROM MAX(a)+AVG(b)+SUMC(c)+max(d)+avg(e)+sumc(f)"),
            (std::vector<std::string>{"a", "b", "c", "d", "e", "f"}));
  // Po kropce RQL.g4 dopuszcza tylko reduktor, ktory i tak odpada jako slowo kluczowe; regula
  // kropki widac dopiero na zwyklej nazwie, takze oddzielonej spacjami.
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y FROM a.b+c . d"), (std::vector<std::string>{"a", "c"}));
}

TEST(serverRouting, sourceStreamsStopAtFirstTerminatorAfterFrom) {
  // Drugie zdanie planu pokazuje, ze analiza sie KONCZY: bez przerwania `FROM b` dodaloby nazwy.
  for (const std::string terminator : {"FILE 'y.dat'", "RETENTION 4", "VOLATILE", "STORAGE MEMORY", "file 'y.dat'",
                                       "retention 4", "volatile", "storage memory"}) {
    SCOPED_TRACE(terminator);
    EXPECT_EQ(routing::extractSourceStreams("SELECT a[0] STREAM y FROM a " + terminator + " SELECT b[0] STREAM z FROM b"),
              (std::vector<std::string>{"a"}));
  }
  // Przed FROM te same slowa niczego nie koncza.
  EXPECT_EQ(routing::extractSourceStreams("DEFAULT VOLATILE STORAGE 'memory' SELECT a[0] STREAM y FROM a"),
            (std::vector<std::string>{"a"}));
  EXPECT_EQ(routing::extractSourceStreams("RULE r ON a WHEN a[0] > 1 DO DUMP -1 TO 1 RETENTION 3 SELECT b[0] STREAM z FROM b"),
            (std::vector<std::string>{"a", "b"}));
}

TEST(serverRouting, sourceStreamsRecognizeOnlyUniformCaseKeywords) {
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y FROM Max+Avg+Sumc+File+Retention+Volatile"),
            (std::vector<std::string>{"Max", "Avg", "Sumc", "File", "Retention", "Volatile"}));
  // `From` i `On` to dla RQL.g4 zwykle nazwy, wiec tekst nie ma ani FROM, ani ON.
  EXPECT_TRUE(routing::extractSourceStreams("SELECT 1 STREAM y From a").empty());
  EXPECT_TRUE(routing::extractSourceStreams("RULE r On a WHEN a[0] > 1 DO DUMP -1 TO 1").empty());
}

TEST(serverRouting, sourceStreamsKeepOrderAndRepetitions) {
  EXPECT_EQ(routing::extractSourceStreams("SELECT 1 STREAM y FROM b+a#b+a"), (std::vector<std::string>{"b", "a", "b", "a"}));
}

TEST(serverRouting, streamResolvesToFirstServingInstanceInSnapshotOrder) {
  // Magistrala nie dopuszcza dwoch wlascicieli jednej nazwy; migawke z takim stanem siejemy wprost.
  const bus::InstanceInfo alfa    = makeInstance("alfa", 101, "alfa.rql", {"dup"});
  const bus::InstanceInfo beta    = makeInstance("beta", 202, "beta.rql", {"dup"});
  const routing::Resolution first = routing::forStream({alfa, beta}, "dup");
  EXPECT_EQ(first.status, routing::Status::Resolved);
  EXPECT_EQ(first.serverName, "alfa");
  const routing::Resolution swapped = routing::forStream({beta, alfa}, "dup");
  EXPECT_EQ(swapped.status, routing::Status::Resolved);
  EXPECT_EQ(swapped.serverName, "beta");
}

TEST(serverRouting, unknownStreamCarriesExactMessage) {
  EXPECT_EQ(routing::forStream(twoInstances(), "nosuch").detail, "nosuch: stream not found on any live instance");
}

TEST(serverRouting, adHocSkipsUnknownNamesWhenOwnerIsUnique) {
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT 1 STREAM tmp FROM srca+nowhere+dsta");
  EXPECT_EQ(resolved.status, routing::Status::Resolved);
  EXPECT_EQ(resolved.serverName, "alfa");
}

TEST(serverRouting, adHocCrossServerListsEachPairOnce) {
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT 1 STREAM tmp FROM dstb+dsta+dstb+srca");
  EXPECT_EQ(resolved.status, routing::Status::CrossServer);
  EXPECT_EQ(resolved.detail, "ad-hoc query crosses a server boundary (dstb@beta, dsta@alfa, srca@alfa)");
}

TEST(serverRouting, adHocUndecidableNamesLiveInstancesAndHint) {
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT 1 STREAM tmp FROM nowhere");
  EXPECT_EQ(resolved.detail, "ad-hoc query names no stream of any live instance (alfa, beta); use --server <name>");
}

TEST(serverRouting, wholeInstanceAmbiguityCountsInstances) {
  std::vector<bus::InstanceInfo> instances = twoInstances();
  instances.push_back(makeInstance("gamma", 303, "gamma.rql", {"dstc"}));
  const routing::Resolution resolved = routing::forSingleTarget(instances);
  EXPECT_EQ(resolved.status, routing::Status::Ambiguous);
  EXPECT_EQ(resolved.detail, "3 live instances (alfa, beta, gamma); use --server <name>");
}

TEST(serverRouting, describeOfEmptySnapshotIsEmpty) { EXPECT_TRUE(routing::describe({}).empty()); }

TEST(serverRouting, describeAndYamlSortByLabelNotRawName) {
  // "#x" < "(unnamed)", ale "" < "#x": kolejnosc rozstrzyga, czy sortowano po etykiecie.
  const std::vector<bus::InstanceInfo> instances{makeInstance("", 1, "a.rql", {"s1"}), makeInstance("#x", 2, "b.rql", {"s2"})};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 5U);
  EXPECT_EQ(lines[2], "#x        | 2   | N    | b.rql | s2");
  EXPECT_EQ(lines[3], "(unnamed) | 1   | N    | a.rql | s1");

  const std::vector<std::string> yaml = routing::describeNamespacesYaml({{.name = "", .instances = instances}});
  ASSERT_EQ(yaml.size(), 17U);
  EXPECT_EQ(yaml[3], "  - name: #x");
  EXPECT_EQ(yaml[10], "  - name: (unnamed)");
}

TEST(serverRouting, describePadsColumnsToWidestValue) {
  // Dlugosc sklejonej listy strumieni (19 + 2 + 1) dalaby kreske dluzsza o trzy znaki.
  const std::vector<std::string> lines =
      routing::describe({makeInstance("alfa", 1234567, "plan.rql", {"averyverylongstream", "s"})});
  ASSERT_EQ(lines.size(), 5U);
  EXPECT_EQ(lines[0], "SERVER | PID     | MODE | QUERY    | STREAMS");
  EXPECT_EQ(lines[1], "-------+---------+------+----------+--------------------");
  EXPECT_EQ(lines[2], "alfa   | 1234567 | N    | plan.rql | averyverylongstream");
  EXPECT_EQ(lines[3], "       |         |      |          | s");
}

TEST(serverRouting, describeShortensQueryPathOnlyWhenShorter) {
  const std::vector<std::string> lines = routing::describe(
      {makeInstance("a", 1, "/p/q.rql", {"s1"}), makeInstance("b", 2, "/plan.rql", {"s2"}),
       makeInstance("c", 3, "deep/dir/tree/plan.rql", {"s3"}), makeInstance("d", 4, "/very/long/plans/dir/x.rql", {"s4"})});
  ASSERT_EQ(lines.size(), 7U);
  EXPECT_EQ(lines[2], "a      | 1   | N    | /p/q.rql               | s1");
  EXPECT_EQ(lines[3], "b      | 2   | N    | /plan.rql              | s2");
  EXPECT_EQ(lines[4], "c      | 3   | N    | deep/dir/tree/plan.rql | s3");
  EXPECT_EQ(lines[5], "d      | 4   | N    | .../dir/x.rql          | s4");
}

TEST(serverRouting, describeNamespacesYamlKeepsGroupOrderAndEscapesQuery) {
  const std::vector<routing::NamespaceGroup> groups{
      {.name = "zeta", .instances = {makeInstance("b", 2, R"(/p/a "q"\c.rql)", {"s"})}},
      {.name = "", .instances = {makeInstance("a", 1, "", {})}}};
  EXPECT_EQ(
      routing::describeNamespacesYaml(groups),
      (std::vector<std::string>{"---", "apiVersion: xqry/v1", "servers:", "  - name: b", R"(    namespace: "zeta")",
                                "    pid: 2", "    modes: N", R"(    query: "/p/a \"q\"\\c.rql")", "    streams:", "      - s",
                                "  - name: a", "    namespace: null", "    pid: 1", "    modes: N", "    streams: []"}));
}

TEST(serverRouting, describeNamespacesYamlFramesDocumentWithoutInstances) {
  EXPECT_EQ(routing::describeNamespacesYaml({}), (std::vector<std::string>{"---", "apiVersion: xqry/v1", "servers: []"}));
  EXPECT_EQ(routing::describeNamespacesYaml({{.name = "", .instances = {}}, {.name = "x", .instances = {}}}),
            (std::vector<std::string>{"---", "apiVersion: xqry/v1", "servers:"}));
}

}  // namespace
