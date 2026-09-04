#include "serverName.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <random>
#include <string>
#include <string_view>

namespace servername {

namespace {

// Dwie listy w stylu nazw kontenerow docker: przymiotnik + nazwisko. Iloczyn (64 x 64 = 4096)
// jest z zapasem wystarczajacy dla liczby serwerow na jednej maszynie; kolizje i tak musza byc
// obsluzone przez wolajacego, bo losowanie nie widzi rejestru.
constexpr std::array<std::string_view, 64> kAdjectives{
    "admiring", "adoring",    "affable",   "amazing",  "awesome",   "blissful",   "bold",       "brave",
    "busy",     "calm",       "clever",    "cool",     "crafty",    "dazzling",   "determined", "eager",
    "eclectic", "elastic",    "elegant",   "epic",     "exciting",  "fervent",    "festive",    "flamboyant",
    "focused",  "friendly",   "frosty",    "gallant",  "gifted",    "goofy",      "gracious",   "great",
    "happy",    "hardcore",   "heuristic", "hopeful",  "hungry",    "infallible", "inspiring",  "jolly",
    "jovial",   "keen",       "kind",      "laughing", "loving",    "lucid",      "magical",    "modest",
    "musing",   "mystifying", "nervous",   "nifty",    "nostalgic", "objective",  "optimistic", "peaceful",
    "pedantic", "pensive",    "practical", "quirky",   "quizzical", "relaxed",    "serene",     "vibrant"};

constexpr std::array<std::string_view, 64> kSurnames{
    "fraenkel", "banach",     "bardeen",      "bassi",      "bell",       "blackwell",   "bohr",       "booth",
    "bose",     "indyk",      "carson",       "cartwright", "kaku",       "clarke",      "codd",       "cori",
    "curie",    "darwin",     "dijkstra",     "dirac",      "edison",     "einstein",    "adler",      "euclid",
    "euler",    "faraday",    "fermat",       "fermi",      "feynman",    "franklin",    "galileo",    "gauss",
    "germain",  "goldberg",   "stern",        "szuwar",     "hamilton",   "hawking",     "heisenberg", "hodgkin",
    "hopper",   "karpinski",  "infeld",       "jackson",    "mayer",      "kaiser",      "kalam",      "kapitsa",
    "knuth",    "kepler",     "apt",          "kopernik",   "kuratowski", "lukasiewicz", "pachocki",   "rejewski",
    "sidor",    "sierpinski", "smoluchowski", "steinhaus",  "tarski",     "tworek",      "ulam",       "zaremba"};

}  // namespace

std::string generate() {
  // Losowosc bierzemy z urzadzenia systemowego, nie z zegara: dwa serwery startowane przez ten
  // sam skrypt w tej samej sekundzie dostawalyby z zegara identyczne ziarno, czyli identyczna
  // nazwe — dokladnie w sytuacji, dla ktorej ten mechanizm istnieje.
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<std::size_t> adjective(0, kAdjectives.size() - 1);
  std::uniform_int_distribution<std::size_t> surname(0, kSurnames.size() - 1);

  std::string retVal(kAdjectives[adjective(gen)]);
  retVal += '_';
  retVal += kSurnames[surname(gen)];
  return retVal;
}

std::string environmentNamespace() {
  const char *env = std::getenv(kNamespaceEnv);
  if (env == nullptr) return {};
  return std::string(env);
}

bool isValid(std::string_view name) {
  if (name.empty() || name.size() > kMaxLength) return false;
  if (name.front() < 'a' || name.front() > 'z') return false;

  return std::ranges::all_of(name,
                             [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-'; });
}

}  // namespace servername
