#include <gtest/gtest.h>

#include <set>
#include <string>

#include "retractor/lib/serverName.hpp"

namespace {

// Nazwa instancji wchodzi do nazw obiektow POSIX shm i do nazwy pliku blokady, wiec
// walidacja nie jest kosmetyczna: znak sciezki albo spacja w nazwie to albo obiekt
// nie do utworzenia, albo blokada w innym miejscu niz sadzi reszta systemu.
TEST(ServerName, rejects_names_unsafe_in_ipc_and_paths) {
  EXPECT_FALSE(servername::isValid(""));
  EXPECT_FALSE(servername::isValid("z spacja"));
  EXPECT_FALSE(servername::isValid("ze/skosem"));
  EXPECT_FALSE(servername::isValid("z.kropka")) << "kropka rozdziela nazwe bazowa od czlonu serwera";
  EXPECT_FALSE(servername::isValid("Wielka"));
  EXPECT_FALSE(servername::isValid("9zaczyna_cyfra"));
  EXPECT_FALSE(servername::isValid(std::string(servername::kMaxLength + 1, 'a')));
}

TEST(ServerName, accepts_expected_shapes) {
  EXPECT_TRUE(servername::isValid("a"));
  EXPECT_TRUE(servername::isValid("nervous_hopper"));
  EXPECT_TRUE(servername::isValid("alfa-2"));
  EXPECT_TRUE(servername::isValid(std::string(servername::kMaxLength, 'a')));
}

// Generator ma produkowac nazwy, ktore przechodza wlasna walidacje — inaczej wylosowana
// nazwa bylaby odrzucana przez sciezke, ktora ma ja przyjac.
TEST(ServerName, generated_names_are_always_valid) {
  for (int i = 0; i < 200; ++i) {
    const std::string name = servername::generate();
    EXPECT_TRUE(servername::isValid(name)) << "wylosowana nazwa nie przechodzi walidacji: " << name;
  }
}

// Losowanie ma realnie rozrozniac instancje. Nie zadamy unikalnosci (o kolizjach decyduje
// wolajacy), ale zbior o jednym elemencie oznaczalby zepsute ziarno.
TEST(ServerName, generated_names_vary) {
  std::set<std::string> seen;
  for (int i = 0; i < 50; ++i)
    seen.insert(servername::generate());
  EXPECT_GT(seen.size(), 1U);
}

}  // namespace
