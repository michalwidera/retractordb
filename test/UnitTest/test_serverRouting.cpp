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
                               const std::vector<std::string> &streams) {
  bus::InstanceInfo retVal;
  retVal.name      = name;
  retVal.pid       = pid;
  retVal.queryFile = queryFile;
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

TEST(serverRouting, identifiersSkipQuotedLiterals) {
  // Napis o tresci cudzej nazwy strumienia nie moze przekierowac zapytania: 'dstb' jest
  // tu literalem, a nie odwolaniem.
  const std::vector<std::string> found = routing::extractIdentifiers("SELECT to_string(dsta[0]:8)+'dstb' STREAM x FROM dsta");
  EXPECT_NE(std::ranges::find(found, "dsta"), found.end());
  EXPECT_EQ(std::ranges::find(found, "dstb"), found.end());
}

TEST(serverRouting, identifiersKeepTrailingDigits) {
  // Nazwy w rodzaju str12 sa w testach integracyjnych regula, nie wyjatkiem.
  const std::vector<std::string> found = routing::extractIdentifiers("SELECT str12[0] STREAM y FROM str12");
  EXPECT_NE(std::ranges::find(found, "str12"), found.end());
  EXPECT_EQ(std::ranges::find(found, "str"), found.end());
}

TEST(serverRouting, adHocResolvesToTheOnlyOwner) {
  const routing::Resolution resolved = routing::forAdHoc(twoInstances(), "SELECT dsta[0]+1 STREAM tmp FROM dsta");
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
                                                 makeInstance("alfa", 101, "alfa.rql", {"srca", "dsta"})};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 2U);
  EXPECT_EQ(lines[0], "alfa 101 alfa.rql srca dsta");
  EXPECT_EQ(lines[1], "beta 202 beta.rql srcb dstb");
}

TEST(serverRouting, describeMarksMissingQueryFile) {
  const std::vector<bus::InstanceInfo> instances{makeInstance("", 101, "", {"dst1"})};
  const std::vector<std::string> lines = routing::describe(instances);
  ASSERT_EQ(lines.size(), 1U);
  EXPECT_EQ(lines[0], "(unnamed) 101 - dst1");
}

}  // namespace
