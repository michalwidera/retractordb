#include "serverRouting.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace routing {

namespace {

/// Czy instancja serwuje strumień o tej nazwie.
bool serves(const bus::InstanceInfo &instance, std::string_view stream) {
  return std::ranges::find(instance.streams, stream) != instance.streams.end();
}

/// Lista etykiet wszystkich instancji, po przecinku — do komunikatu o dwuznaczności.
std::string labelList(const std::vector<bus::InstanceInfo> &instances) {
  std::string retVal;
  for (const auto &instance : instances) {
    if (!retVal.empty()) retVal += ", ";
    retVal += instanceLabel(instance.name);
  }
  return retVal;
}

}  // namespace

std::string instanceLabel(std::string_view name) { return name.empty() ? std::string{"(unnamed)"} : std::string{name}; }

std::vector<std::string> extractIdentifiers(std::string_view query) {
  std::vector<std::string> retVal;
  std::string token;
  bool inLiteral{false};

  auto flush = [&retVal, &token]() {
    if (!token.empty()) retVal.push_back(token);
    token.clear();
  };

  for (const char c : query) {
    if (c == '\'') {
      flush();
      inLiteral = !inLiteral;
      continue;
    }
    if (inLiteral) continue;

    const auto uc    = static_cast<unsigned char>(c);
    const bool head  = std::isalpha(uc) != 0 || c == '_';
    const bool inner = head || std::isdigit(uc) != 0;
    if (token.empty() ? head : inner)
      token.push_back(c);
    else
      flush();
  }
  flush();
  return retVal;
}

Resolution forStream(const std::vector<bus::InstanceInfo> &instances, std::string_view stream) {
  Resolution retVal;
  for (const auto &instance : instances)
    if (serves(instance, stream)) {
      retVal.serverName = instance.name;
      return retVal;
    }

  // Kod wyjścia tej ścieżki musi zostać kodem "nie ma takiego strumienia", a nie "serwer
  // milczy": issue_215 celowo rozdzielił te dwie diagnozy, bo prowadzą do różnych napraw.
  retVal.status = Status::StreamNotFound;
  retVal.detail = std::string{stream} + ": stream not found on any live instance";
  return retVal;
}

Resolution forAdHoc(const std::vector<bus::InstanceInfo> &instances, std::string_view query) {
  Resolution retVal;
  const bus::InstanceInfo *owner{nullptr};
  bool crossed{false};
  std::vector<std::string> reached;

  for (const auto &token : extractIdentifiers(query))
    for (const auto &instance : instances) {
      if (!serves(instance, token)) continue;
      // Ta sama nazwa pada w zapytaniu wielokrotnie (`dsta[0]` i `FROM dsta`), a komunikat
      // ma wymieniac strumienie, nie ich wystapienia.
      const std::string entry = token + "@" + instanceLabel(instance.name);
      if (std::ranges::find(reached, entry) == reached.end()) reached.push_back(entry);
      if (owner == nullptr)
        owner = &instance;
      else if (owner != &instance)
        crossed = true;
    }

  if (crossed) {
    std::string list;
    for (const auto &entry : reached) {
      if (!list.empty()) list += ", ";
      list += entry;
    }
    retVal.status = Status::CrossServer;
    retVal.detail = "ad-hoc query crosses a server boundary (" + list + ")";
    return retVal;
  }
  if (owner == nullptr) {
    // Zapytanie ad-hoc bez ani jednej znanej nazwy strumienia jest nierozstrzygalne: nie ma
    // żadnej przesłanki, do której instancji miałoby trafić, a zgadywanie zmieniłoby plan
    // przypadkowego serwera.
    retVal.status = Status::Ambiguous;
    retVal.detail = "ad-hoc query names no stream of any live instance (" + labelList(instances) + "); use --server <name>";
    return retVal;
  }

  retVal.serverName = owner->name;
  return retVal;
}

Resolution forSingleTarget(const std::vector<bus::InstanceInfo> &instances) {
  Resolution retVal;
  if (instances.size() <= 1) {
    // Brak instancji => nazwa pusta, czyli nazwy historyczne. Zepsuta albo pusta magistrala
    // nie może unieruchomić klienta: wtedy zachowuje się dokładnie tak jak przed etapem 2c.
    if (!instances.empty()) retVal.serverName = instances.front().name;
    return retVal;
  }

  retVal.status = Status::Ambiguous;
  retVal.detail = std::to_string(instances.size()) + " live instances (" + labelList(instances) + "); use --server <name>";
  return retVal;
}

std::vector<std::string> describe(const std::vector<bus::InstanceInfo> &instances) {
  std::vector<std::string> retVal;
  retVal.reserve(instances.size());
  for (const auto &instance : instances) {
    std::string line = instanceLabel(instance.name) + " " + std::to_string(instance.pid) + " " +
                       (instance.queryFile.empty() ? std::string{"-"} : instance.queryFile);
    for (const auto &stream : instance.streams)
      line += " " + stream;
    retVal.push_back(std::move(line));
  }
  // Sortowanie po całej linii, czyli w pierwszej kolejności po nazwie instancji: kolejność
  // slotów w segmencie zależy od kolejności startów, a wyjście ma być powtarzalne.
  std::ranges::sort(retVal);
  return retVal;
}

}  // namespace routing
