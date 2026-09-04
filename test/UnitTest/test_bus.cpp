#include <gtest/gtest.h>

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <boost/interprocess/shared_memory_object.hpp>

#include "retractor/lib/bus.hpp"
#include "retractor/lib/serverName.hpp"

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

  const auto claimed = first.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"srca", "dsta"}});
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

  ASSERT_EQ(first.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"srca", "dst"}}).status,
            bus::ClaimStatus::Claimed);

  const auto refused = second.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"srcb", "dst"}});
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

  ASSERT_EQ(historic.claim({.name = "", .queryFile = "query.rql", .streams = {"dst"}}).status, bus::ClaimStatus::Claimed);

  const auto refused = named.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"dst"}});
  EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
  EXPECT_TRUE(refused.ownerName.empty());
}

TEST_F(BusFixture, DisjointStreamsCoexist) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);

  ASSERT_EQ(first.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"srca", "dsta"}}).status,
            bus::ClaimStatus::Claimed);
  EXPECT_EQ(second.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"srcb", "dstb"}}).status,
            bus::ClaimStatus::Claimed);

  const auto instances = first.instances();
  ASSERT_EQ(instances.size(), 2U);
  EXPECT_EQ(streamsOf(instances, "alfa"), (std::vector<std::string>{"srca", "dsta"}));
  EXPECT_EQ(streamsOf(instances, "beta"), (std::vector<std::string>{"srcb", "dstb"}));
}

TEST_F(BusFixture, ReleaseFreesTheName) {
  bus::Bus second(kTestSegment);
  {
    bus::Bus first(kTestSegment);
    ASSERT_EQ(first.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"dst"}}).status, bus::ClaimStatus::Claimed);
    ASSERT_EQ(second.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"dst"}}).status, bus::ClaimStatus::Conflict);
  }  // destruktor zwalnia slot

  EXPECT_EQ(second.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"dst"}}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(second.instances().size(), 1U);
}

// Powtorne roszczenie tej samej instancji zastepuje jej wlasny slot, a nie doklada drugiego
// -- inaczej instancja kolidowalaby sama ze soba.
TEST_F(BusFixture, ReclaimReplacesOwnSlot) {
  bus::Bus instance(kTestSegment);

  ASSERT_EQ(instance.claim({.name = "alfa", .queryFile = "one.rql", .streams = {"dst"}}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(instance.claim({.name = "alfa", .queryFile = "two.rql", .streams = {"dst", "other"}}).status,
            bus::ClaimStatus::Claimed);

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
  EXPECT_EQ(instance.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = tooMany}).status, bus::ClaimStatus::TooLarge);

  const std::string tooLong(bus::kStreamNameSize, 'x');
  const auto refused = instance.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {tooLong}});
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
  ASSERT_EQ(instance.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {longest}}).status, bus::ClaimStatus::Claimed);

  const auto instances = instance.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].streams, (std::vector<std::string>{longest}));
}

// Sedno rozszerzenia na ad-hoc: nazwa powolana w locie dochodzi do wlasnego slotu i od tej
// chwili jest widoczna dla pozostalych instancji.
TEST_F(BusFixture, ClaimAdditionalExtendsOwnSlot) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);

  ASSERT_EQ(first.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"srca"}}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(first.claimAdditional({"adhoc1"}).status, bus::ClaimStatus::Claimed);

  EXPECT_EQ(streamsOf(first.instances(), "alfa"), (std::vector<std::string>{"srca", "adhoc1"}));
  EXPECT_EQ(second.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"adhoc1"}}).status, bus::ClaimStatus::Conflict);
}

// Nazwa nalezaca do innej zywej instancji jest odmowa ze wskazaniem wlasciciela -- i, co
// wazniejsze, odmowa NIE rusza wlasnego slotu. Gdyby claimAdditional bylo zwyklym claim(),
// zaczynajacym od release(), serwer straciłby po odmowie roszczenie takze na nazwy,
// ktore juz obsluguje.
TEST_F(BusFixture, ClaimAdditionalConflictLeavesOwnSlotIntact) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);

  ASSERT_EQ(first.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"srca", "dsta"}}).status,
            bus::ClaimStatus::Claimed);
  ASSERT_EQ(second.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"srcb", "dstb"}}).status,
            bus::ClaimStatus::Claimed);

  const auto refused = first.claimAdditional({"nowy", "dstb"});
  EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
  EXPECT_EQ(refused.stream, "dstb");
  EXPECT_EQ(refused.ownerName, "beta");
  EXPECT_EQ(refused.ownerPid, static_cast<std::int32_t>(getpid()));

  // Slot nietkniety: ani odrzucona nazwa, ani ta, ktora byla z nia w jednym zapytaniu.
  EXPECT_EQ(streamsOf(first.instances(), "alfa"), (std::vector<std::string>{"srca", "dsta"}));
  EXPECT_EQ(first.instances().size(), 2U);
}

// Wlasna nazwa nie koliduje sama ze soba: powtorzone zapytanie ad-hoc ma przechodzic,
// a slot nie ma rosnac o duplikaty.
TEST_F(BusFixture, ClaimAdditionalIsIdempotent) {
  bus::Bus instance(kTestSegment);

  ASSERT_EQ(instance.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"srca"}}).status, bus::ClaimStatus::Claimed);
  ASSERT_EQ(instance.claimAdditional({"adhoc1"}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(instance.claimAdditional({"srca", "adhoc1"}).status, bus::ClaimStatus::Claimed);

  EXPECT_EQ(streamsOf(instance.instances(), "alfa"), (std::vector<std::string>{"srca", "adhoc1"}));
}

// Bez wlasnego slotu nie ma czego rozszerzac. Wynik Unavailable jest odrozniony od Conflict,
// bo sciezka ad-hoc traktuje go inaczej: przepuszcza zapytanie i zapisuje w logu, ze
// rozlacznosc nie byla wtedy egzekwowana.
TEST_F(BusFixture, ClaimAdditionalWithoutSlotIsUnavailable) {
  bus::Bus instance(kTestSegment);
  ASSERT_TRUE(instance.attached());

  EXPECT_EQ(instance.claimAdditional({"adhoc1"}).status, bus::ClaimStatus::Unavailable);
  EXPECT_TRUE(instance.instances().empty());

  // Pusta lista jest operacja pusta takze wtedy, gdy slotu nie ma.
  EXPECT_EQ(instance.claimAdditional({}).status, bus::ClaimStatus::Claimed);
}

// Pojemnosc slotu obowiazuje takze przy dokladaniu, i tak samo nie wolno jej przekroczyc
// cicho: slot z obcieta lista przestalby odpowiadac na pytanie o rozlacznosc.
TEST_F(BusFixture, ClaimAdditionalRespectsSlotCapacity) {
  bus::Bus instance(kTestSegment);

  std::vector<std::string> full;
  for (std::size_t i = 0; i < bus::kMaxStreams; ++i)
    full.push_back("s" + std::to_string(i));
  ASSERT_EQ(instance.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = full}).status, bus::ClaimStatus::Claimed);

  EXPECT_EQ(instance.claimAdditional({"jeszczejeden"}).status, bus::ClaimStatus::TooLarge);
  EXPECT_EQ(streamsOf(instance.instances(), "alfa").size(), bus::kMaxStreams);

  const std::string tooLong(bus::kStreamNameSize, 'x');
  const auto refused = instance.claimAdditional({tooLong});
  EXPECT_EQ(refused.status, bus::ClaimStatus::TooLarge);
  EXPECT_EQ(refused.stream, tooLong);
}

// Serwer zabity, ale niezebrany przez rodzica, zostaje procesem zombie: /proc/<pid>/stat
// istnieje nadal, razem z niezmienionym starttime. Bez sprawdzenia stanu taki slot
// trzymalby swoje nazwy strumieni az do wait() rodzica -- czyli dowolnie dlugo, bo to
// rodzic decyduje, kiedy zbierze potomka.
TEST_F(BusFixture, ZombieSlotIsFreeAgain) {
  bus::Bus parent(kTestSegment);
  ASSERT_TRUE(parent.attached());

  const pid_t child = fork();
  ASSERT_NE(child, -1);

  if (child == 0) {
    // Potomek roszczy nazwe i zostaje przy zyciu. Zadnych asercji GTest tutaj: to nie jest
    // proces testowy, a jego jedynym zadaniem jest zginac ze slotem, ktorego nikt nie zwolnil.
    bus::Bus mine(kTestSegment);
    mine.claim({.name = "zombie", .queryFile = "zombie.rql", .streams = {"dstz"}});
    for (;;)
      pause();
    _exit(0);
  }

  // Rodzic czeka na widoczny slot potomka: dopiero wtedy zabicie go zostawia slot do sprzatania.
  const auto childVisible = [&] {
    const auto instances = parent.instances();
    return std::ranges::any_of(instances, [&](const auto &i) { return i.pid == child; });
  };
  for (int attempt = 0; attempt < 200 && !childVisible(); ++attempt)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(childVisible()) << "potomek nie zdazyl zaroscic slotu";

  const std::uint64_t childStart = bus::processStartTime(child);
  ASSERT_NE(childStart, 0U);

  // SIGKILL bez waitpid: od tej chwili potomek jest zombie i pozostanie nim do konca testu.
  ASSERT_EQ(kill(child, SIGKILL), 0);
  for (int attempt = 0; attempt < 200 && bus::isProcessAlive(child, childStart); ++attempt)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  EXPECT_FALSE(bus::isProcessAlive(child, childStart));
  EXPECT_TRUE(parent.instances().empty());

  // Sedno higienizacji: pierwsze roszczenie po smierci potomka sprzata jego slot i przejmuje
  // nazwe. Bez tego magistrala trzymalaby 'dstz' do chwili, gdy ktos zbierze zombie.
  EXPECT_EQ(parent.claim({.name = "alfa", .queryFile = "alfa.rql", .streams = {"dstz"}}).status, bus::ClaimStatus::Claimed);
  EXPECT_EQ(streamsOf(parent.instances(), "alfa"), (std::vector<std::string>{"dstz"}));

  // Zombie zbieramy dopiero teraz: test nie ma prawa zostawic go po sobie.
  int childStatus = 0;
  ASSERT_EQ(waitpid(child, &childStatus, 0), child);
}

// Reguly wyboru wlasciciela przy dostarczaniu zestawu zapytan dzialajacemu serwisowi.
// Czysta funkcja nad gotowa migawka -- bez pamieci dzielonej i bez serwerow.
namespace {

std::vector<bus::InstanceInfo> snapshotOf() {
  return {bus::InstanceInfo{
              .name = "alfa", .pid = 101, .queryFile = "alfa.rql", .counterPath = "/tmp/alfa.cnt", .streams = {"srca", "dst"}},
          bus::InstanceInfo{
              .name = "beta", .pid = 102, .queryFile = "beta.rql", .counterPath = "/tmp/beta.cnt", .streams = {"srcb", "dstb"}},
          bus::InstanceInfo{.name = "", .pid = 103, .queryFile = "hist.rql", .streams = {"srce", "dste"}}};
}

}  // namespace

TEST(BusForeignOwner, ReportsOwnerOutsideSelf) {
  const auto owner = bus::findForeignOwner(snapshotOf(), "beta", {"srcb", "dst"});
  ASSERT_TRUE(owner.has_value());
  EXPECT_EQ(owner->stream, "dst");
  EXPECT_EQ(owner->instance, "alfa");
  EXPECT_EQ(owner->pid, 101);
}

// Sedno reguly: nazwy WLASNEJ instancji nie sa kolizja. Restart serwisu zwalnia jego slot,
// wiec zestaw dostarczany temu samemu serwisowi musi przejsc, choc jego nazwy sa w magistrali.
TEST(BusForeignOwner, OwnStreamsAreNotAConflict) {
  EXPECT_FALSE(bus::findForeignOwner(snapshotOf(), "beta", {"srcb", "dstb"}).has_value());
  EXPECT_FALSE(bus::findForeignOwner(snapshotOf(), "alfa", {"srca", "dst"}).has_value());
}

// Instancja bezimienna bierze udzial w regule na tych samych prawach: dostarczenie do niej
// pomija jej wlasne nazwy, a dostarczenie do nazwanej instancji widzi jej nazwy jako cudze.
TEST(BusForeignOwner, UnnamedInstanceTakesPart) {
  const auto owner = bus::findForeignOwner(snapshotOf(), "alfa", {"dste"});
  ASSERT_TRUE(owner.has_value());
  EXPECT_EQ(owner->instance, "");
  EXPECT_EQ(owner->pid, 103);

  EXPECT_FALSE(bus::findForeignOwner(snapshotOf(), "", {"srce", "dste"}).has_value());
}

TEST(BusForeignOwner, DisjointSetAndEmptyInputsPass) {
  EXPECT_FALSE(bus::findForeignOwner(snapshotOf(), "gamma", {"nowy1", "nowy2"}).has_value());
  EXPECT_FALSE(bus::findForeignOwner(snapshotOf(), "gamma", {}).has_value());
  EXPECT_FALSE(bus::findForeignOwner({}, "gamma", {"dst"}).has_value());
}

TEST(BusForeignCounterOwner, ReportsForeignCounterBeforeDelivery) {
  const auto owner = bus::findForeignCounterOwner(snapshotOf(), "beta", "/tmp/alfa.cnt");
  ASSERT_TRUE(owner.has_value());
  EXPECT_EQ(owner->path, "/tmp/alfa.cnt");
  EXPECT_EQ(owner->instance, "alfa");
  EXPECT_EQ(owner->pid, 101);
}

TEST(BusForeignCounterOwner, OwnDistinctAndEmptyCountersPass) {
  EXPECT_FALSE(bus::findForeignCounterOwner(snapshotOf(), "alfa", "/tmp/alfa.cnt").has_value());
  EXPECT_FALSE(bus::findForeignCounterOwner(snapshotOf(), "gamma", "/tmp/gamma.cnt").has_value());
  EXPECT_FALSE(bus::findForeignCounterOwner(snapshotOf(), "gamma", "").has_value());
}

// Licznik rotacji nie jest nazwa strumienia, wiec rozlacznosc nazw go nie chroni. Dwie
// instancje na jednym pliku wczytuja te sama wartosc i zapisuja te sama wartosc+1 (licznik
// aktualizuje sie dopiero w destruktorze PersistentCounter), czyli gubia rotacje i nadpisuja
// sobie archiwa. Nazwy strumieni sa tu ROZLACZNE -- kolizja dotyczy wylacznie licznika.
TEST_F(BusFixture, SharedRotationCounterIsRefused) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);

  ASSERT_EQ(
      first.claim({.name = "alfa", .queryFile = "alfa.rql", .counterPath = "/var/lib/rdb/rot.cnt", .streams = {"dsta"}}).status,
      bus::ClaimStatus::Claimed);

  const auto refused =
      second.claim({.name = "beta", .queryFile = "beta.rql", .counterPath = "/var/lib/rdb/rot.cnt", .streams = {"dstb"}});
  EXPECT_EQ(refused.status, bus::ClaimStatus::CounterConflict);
  EXPECT_EQ(refused.detail, "/var/lib/rdb/rot.cnt");
  EXPECT_EQ(refused.ownerName, "alfa");
  EXPECT_EQ(refused.ownerPid, static_cast<std::int32_t>(getpid()));

  // Odmowa nie zostawia po sobie slotu.
  EXPECT_EQ(first.instances().size(), 1U);
}

// Rozne pliki licznika wspolistnieja, a plan BEZ :ROTATION ma sciezke pusta i nie koliduje
// z niczym -- inaczej dwie zwykle instancje blokowalyby sie nawzajem na pustym napisie.
TEST_F(BusFixture, DistinctAndAbsentCountersCoexist) {
  bus::Bus first(kTestSegment);
  bus::Bus second(kTestSegment);
  bus::Bus third(kTestSegment);

  ASSERT_EQ(first.claim({.name = "alfa", .queryFile = "a.rql", .counterPath = "/tmp/a.cnt", .streams = {"dsta"}}).status,
            bus::ClaimStatus::Claimed);
  EXPECT_EQ(second.claim({.name = "beta", .queryFile = "b.rql", .counterPath = "/tmp/b.cnt", .streams = {"dstb"}}).status,
            bus::ClaimStatus::Claimed);
  EXPECT_EQ(third.claim({.name = "gamma", .queryFile = "g.rql", .streams = {"dstg"}}).status, bus::ClaimStatus::Claimed);

  EXPECT_EQ(first.instances().size(), 3U);
}

// Slot niesie nazwe jednostki systemd i sciezke licznika: to na ich podstawie operator
// dowiaduje sie, ktora jednostke zatrzymac, zeby zwolnic kolidujaca nazwe.
TEST_F(BusFixture, SlotCarriesUnitAndCounterPath) {
  bus::Bus instance(kTestSegment);

  ASSERT_EQ(instance
                .claim({.name        = "alfa",
                        .queryFile   = "alfa.rql",
                        .unit        = "xretractor-alfa.service",
                        .counterPath = "/var/lib/rdb/alfa.cnt",
                        .streams     = {"dsta"}})
                .status,
            bus::ClaimStatus::Claimed);

  const auto instances = instance.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].unit, "xretractor-alfa.service");
  EXPECT_EQ(instances[0].counterPath, "/var/lib/rdb/alfa.cnt");

  // Instancja bez systemd i bez rotacji ma oba pola puste, a nie smieci po poprzedniku slotu.
  instance.release();
  ASSERT_EQ(instance.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"dstb"}}).status, bus::ClaimStatus::Claimed);
  const auto plain = instance.instances();
  ASSERT_EQ(plain.size(), 1U);
  EXPECT_TRUE(plain[0].unit.empty());
  EXPECT_TRUE(plain[0].counterPath.empty());
}

// Maska trybow pracy jest wlasnoscia uruchomienia, nie planu, wiec slot musi ja niesc obok
// pliku zapytan -- i czyscic przy przejeciu slotu, zeby po instancji realtime nie zostal
// bit realtime u jej nastepcy.
TEST_F(BusFixture, SlotCarriesRunModes) {
  bus::Bus instance(kTestSegment);

  ASSERT_EQ(instance
                .claim({.name      = "alfa",
                        .queryFile = "alfa.rql",
                        .modes     = bus::mode::kRealTime | bus::mode::kService,
                        .streams   = {"dsta"}})
                .status,
            bus::ClaimStatus::Claimed);

  const auto instances = instance.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].modes, bus::mode::kRealTime | bus::mode::kService);

  instance.release();
  ASSERT_EQ(instance.claim({.name = "beta", .queryFile = "beta.rql", .streams = {"dstb"}}).status, bus::ClaimStatus::Claimed);
  const auto plain = instance.instances();
  ASSERT_EQ(plain.size(), 1U);
  EXPECT_EQ(plain[0].modes, 0U);
}

// Sciezka dluzsza niz pole slotu jest odmowa, nie cichym obcieciem: obciety napis zrownalby
// dwa rozne pliki licznika albo rozdzielil jeden.
TEST_F(BusFixture, OversizedCounterPathIsRefused) {
  bus::Bus instance(kTestSegment);

  const std::string tooLong(bus::kCounterPathSize, 'p');
  EXPECT_EQ(instance.claim({.name = "alfa", .queryFile = "alfa.rql", .counterPath = tooLong, .streams = {"dsta"}}).status,
            bus::ClaimStatus::TooLarge);
  EXPECT_TRUE(instance.instances().empty());
}

// Nazwa segmentu niesie wersje ukladu i musi isc w gore razem z nia. Bez tego stary segment
// zostaje w /dev/shm po podmianie binarki, a instancja, ktora odmowi sie do niego podlaczyc,
// startuje BEZ egzekwowania rozlacznosci -- awaria cicha az do pierwszej kolizji.
//
// Podkreslenie zamiast kropki jest czescia kontraktu: obiekty IPC instancji nazywaja sie
// "<obiekt>.<nazwa instancji>", wiec segment z kropka wpadlby pod wzorce sprzatajace /dev/shm/*.<nazwa>.
TEST(BusSegmentName, CarriesLayoutVersionAndAvoidsInstanceNamespace) {
  EXPECT_EQ(bus::kSegmentName, "xrdbbus_v3");
  EXPECT_EQ(bus::kSegmentName.find('.'), std::string_view::npos);
}

// Przestrzen nazw uruchomienia rozdziela magistrale. To jest mechanizm, dzieki ktoremu
// dwa zestawy testow uzywajace tych samych nazw strumieni moga biec obok siebie: bez
// wlasnego segmentu drugi start odpadlby z ClaimStatus::Conflict.
//
// Wartosc niepoprawna NIE MOZE trafic do nazwy obiektu /dev/shm. Launchery odrzucaja ja
// wczesniej z komunikatem, a tutaj obowiazuje zasada odwrotna do cichej zguby izolacji:
// zly znak dalby blad otwarcia segmentu, ktory przeszedlby jako zwykle "magistrala
// niedostepna" — czyli fail-open bez sladu w logu testu.
class BusSegmentNamespace : public ::testing::Test {
 protected:
  void TearDown() override { unsetenv(servername::kNamespaceEnv); }
};

TEST_F(BusSegmentNamespace, AppendsNamespaceToSegmentName) {
  ASSERT_EQ(setenv(servername::kNamespaceEnv, "it07", 1), 0);
  EXPECT_EQ(bus::segmentName(), std::string(bus::kSegmentName) + "_it07");
}

TEST_F(BusSegmentNamespace, UnsetOrEmptyKeepsProductionSegment) {
  ASSERT_EQ(unsetenv(servername::kNamespaceEnv), 0);
  EXPECT_EQ(bus::segmentName(), bus::kSegmentName);
  ASSERT_EQ(setenv(servername::kNamespaceEnv, "", 1), 0);
  EXPECT_EQ(bus::segmentName(), bus::kSegmentName);
}

TEST_F(BusSegmentNamespace, InvalidValueNeverReachesTheObjectName) {
  for (const char *bad : {"z/skosem", "z.kropka", "Wielka", "9cyfra", "z spacja"}) {
    ASSERT_EQ(setenv(servername::kNamespaceEnv, bad, 1), 0);
    EXPECT_EQ(bus::segmentName(), bus::kSegmentName) << "niepoprawna wartosc trafila do nazwy segmentu: " << bad;
  }
}
