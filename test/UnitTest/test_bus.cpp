#include <gtest/gtest.h>

#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

#include <boost/interprocess/shared_memory_object.hpp>

#include "retractor/lib/bus.hpp"

namespace {

namespace IPC = boost::interprocess;

// Testy pracuja na WLASNYM segmencie, nie na "xrdbbus": magistrala dzialajacych serwerow
// nie moze byc kasowana ani zasmiecana przez przebieg testu.
constexpr const char *kTestSegment = "xrdbbus_ut";

// Kazdy test zaczyna i konczy sie bez sladu po sobie w /dev/shm, niezaleznie od wyniku.
class BusFixture : public ::testing::Test {
 protected:
  void SetUp() override { IPC::shared_memory_object::remove(kTestSegment); }
  void TearDown() override { IPC::shared_memory_object::remove(kTestSegment); }
};

std::vector<std::string> streamsOf(const std::vector<bus::InstanceInfo> &instances, const std::string &name) {
  const auto it = std::ranges::find_if(instances, [&](const auto &i) { return i.name == name; });
  return it != instances.end() ? it->streams : std::vector<std::string>{};
}

}  // namespace

// Podstawa calej reszty: proces widzi siebie jako zywego, a niezgodny czas startu czyni
// wpis martwym. To wlasnie ta niezgodnosc odroznia zywy slot od slotu po procesie, ktoremu
// jadro nadalo wczesniej ten sam, zwolniony PID -- sam kill(pid, 0) tego nie rozstrzyga.
TEST_F(BusFixture, LivenessRequiresMatchingStartTime) {
  const auto self           = static_cast<std::int32_t>(getpid());
  const std::uint64_t start = bus::processStartTime(self);

  EXPECT_NE(start, 0U);
  EXPECT_TRUE(bus::isProcessAlive(self, start));
  EXPECT_FALSE(bus::isProcessAlive(self, start + 1));
  EXPECT_FALSE(bus::isProcessAlive(0, start));
  EXPECT_FALSE(bus::isProcessAlive(-1, start));
}

TEST_F(BusFixture, ClaimPublishesInstance) {
  bus::Bus first(kTestSegment);
  ASSERT_TRUE(first.attached());

  const auto claimed = first.claim("alfa", "alfa.rql", {"srca", "dsta"});
  EXPECT_EQ(claimed.status, bus::ClaimStatus::Claimed);

  const auto instances = first.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].name, "alfa");
  EXPECT_EQ(instances[0].pid, static_cast<std::int32_t>(getpid()));
  EXPECT_EQ(instances[0].queryFile, "alfa.rql");
  EXPECT_EQ(instances[0].streams, (std::vector<std::string>{"srca", "dsta"}));
}

// Sedno etapu 2b: kolizja nazwy strumienia jest odmowa, a odmowa wskazuje wlasciciela.
TEST_F(BusFixture, CollidingStreamIsRefusedWithOwner) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);
  ASSERT_TRUE(first.attached());
  ASSERT_TRUE(second.attached());

  ASSERT_EQ(first.claim("alfa", "alfa.rql", {"srca", "dst"}).status, bus::ClaimStatus::Claimed);

  const auto refused = second.claim("beta", "beta.rql", {"srcb", "dst"});
  EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
  EXPECT_EQ(refused.stream, "dst");
  EXPECT_EQ(refused.ownerName, "alfa");
  EXPECT_EQ(refused.ownerPid, static_cast<std::int32_t>(getpid()));

  // Odmowa nie zostawia po sobie slotu.
  EXPECT_EQ(first.instances().size(), 1U);
}

// Instancja bezimienna (tryb historyczny, bez --name) tez trzyma slot i tez uczestniczy
// w rozlacznosci -- inaczej kolizja nazwana<->bezimienna przechodzilaby niewykryta.
TEST_F(BusFixture, UnnamedInstanceTakesPartInUniqueness) {
  bus::Bus historic(kTestSegment);
  bus::Bus named(kTestSegment);

  ASSERT_EQ(historic.claim("", "query.rql", {"dst"}).status, bus::ClaimStatus::Claimed);

  const auto refused = named.claim("alfa", "alfa.rql", {"dst"});
  EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
  EXPECT_TRUE(refused.ownerName.empty());
}

TEST_F(BusFixture, DisjointStreamsCoexist) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);

  ASSERT_EQ(first.claim("alfa", "alfa.rql", {"srca", "dsta"}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(second.claim("beta", "beta.rql", {"srcb", "dstb"}).status, bus::ClaimStatus::Claimed);

  const auto instances = first.instances();
  ASSERT_EQ(instances.size(), 2U);
  EXPECT_EQ(streamsOf(instances, "alfa"), (std::vector<std::string>{"srca", "dsta"}));
  EXPECT_EQ(streamsOf(instances, "beta"), (std::vector<std::string>{"srcb", "dstb"}));
}

TEST_F(BusFixture, ReleaseFreesTheName) {
  bus::Bus second(kTestSegment);
  {
    bus::Bus first(kTestSegment);
    ASSERT_EQ(first.claim("alfa", "alfa.rql", {"dst"}).status, bus::ClaimStatus::Claimed);
    ASSERT_EQ(second.claim("beta", "beta.rql", {"dst"}).status, bus::ClaimStatus::Conflict);
  }  // destruktor zwalnia slot

  EXPECT_EQ(second.claim("beta", "beta.rql", {"dst"}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(second.instances().size(), 1U);
}

// Powtorne roszczenie tej samej instancji zastepuje jej wlasny slot, a nie doklada drugiego
// -- inaczej instancja kolidowalaby sama ze soba.
TEST_F(BusFixture, ReclaimReplacesOwnSlot) {
  bus::Bus instance(kTestSegment);

  ASSERT_EQ(instance.claim("alfa", "one.rql", {"dst"}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(instance.claim("alfa", "two.rql", {"dst", "other"}).status, bus::ClaimStatus::Claimed);

  const auto instances = instance.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].queryFile, "two.rql");
  EXPECT_EQ(instances[0].streams, (std::vector<std::string>{"dst", "other"}));
}

// Przekroczenie pojemnosci slotu jest bledem, a nie cichym zawieszeniem gwarancji:
// slot z obcieta lista strumieni nie moglby juz odpowiadac na pytanie o rozlacznosc.
TEST_F(BusFixture, OversizedPlanIsRefused) {
  bus::Bus instance(kTestSegment);

  std::vector<std::string> tooMany;
  for (std::size_t i = 0; i <= bus::kMaxStreams; ++i)
    tooMany.push_back("s" + std::to_string(i));
  EXPECT_EQ(instance.claim("alfa", "alfa.rql", tooMany).status, bus::ClaimStatus::TooLarge);

  const std::string tooLong(bus::kStreamNameSize, 'x');
  const auto refused = instance.claim("alfa", "alfa.rql", {tooLong});
  EXPECT_EQ(refused.status, bus::ClaimStatus::TooLarge);
  EXPECT_EQ(refused.stream, tooLong);

  // Zadna z odmow nie zajela slotu.
  EXPECT_TRUE(instance.instances().empty());
}

// Nazwa graniczna (o jeden znak krotsza niz pojemnosc pola) musi przejsc W CALOSCI --
// milczace obciecie zamienialoby dwie rozne nazwy w jedna i falszowalo kolizje.
TEST_F(BusFixture, LongestAllowedStreamNameSurvivesRoundTrip) {
  bus::Bus instance(kTestSegment);

  const std::string longest(bus::kStreamNameSize - 1, 'y');
  ASSERT_EQ(instance.claim("alfa", "alfa.rql", {longest}).status, bus::ClaimStatus::Claimed);

  const auto instances = instance.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].streams, (std::vector<std::string>{longest}));
}
