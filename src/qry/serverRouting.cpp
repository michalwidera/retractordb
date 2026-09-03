#include "serverRouting.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
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

std::vector<std::string> extractSourceStreams(std::string_view query) {
  // RQL.g4 leksuje slowa kluczowe WIELKOSCIOWO: `FROM: 'FROM'|'from'`, i tak samo MIN, MAX, AVG,
  // SUMC, FILE, RETENTION, VOLATILE i STORAGE. Pisownia mieszana NIE jest slowem kluczowym, tylko
  // zwykla nazwa -- grammar mowi to wprost przy regule stream_fn_call ("`Min` pozostaje zwykla
  // nazwa"). Skladanie tokenu do lowercase odbieraloby wiec strumieniowi nazwanemu `Min` albo
  // `From` szanse na rozpoznanie, a routing gubilby jego wlasciciela.
  const auto isKeyword = [](std::string_view token, std::string_view upper, std::string_view lower) {
    return token == upper || token == lower;
  };

  std::vector<std::string> retVal;
  bool inLiteral{false};
  bool inLineComment{false};
  std::size_t blockCommentDepth{0};
  bool inFrom{false};
  char previousSignificant{'\0'};

  for (std::size_t i = 0; i < query.size();) {
    const char c    = query[i];
    const char next = i + 1 < query.size() ? query[i + 1] : '\0';

    if (inLineComment) {
      if (c == '\n') inLineComment = false;
      ++i;
      continue;
    }
    if (blockCommentDepth != 0) {
      if (c == '/' && next == '*') {
        ++blockCommentDepth;
        i += 2;
      } else if (c == '*' && next == '/') {
        --blockCommentDepth;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (inLiteral) {
      if (c == '\\' && next != '\0') {
        i += 2;
      } else {
        if (c == '\'') inLiteral = false;
        ++i;
      }
      continue;
    }
    if (c == '/' && next == '/') {
      inLineComment = true;
      i += 2;
      continue;
    }
    if (c == '/' && next == '*') {
      blockCommentDepth = 1;
      i += 2;
      continue;
    }
    if (c == '\'') {
      inLiteral = true;
      ++i;
      continue;
    }

    const auto uc = static_cast<unsigned char>(c);
    if (std::isalpha(uc) != 0) {
      const std::size_t begin = i++;
      while (i < query.size()) {
        const char inner = query[i];
        const auto uci   = static_cast<unsigned char>(inner);
        if (std::isalnum(uci) == 0 && inner != '_' && inner != '$') break;
        ++i;
      }

      const std::string token(query.substr(begin, i - begin));

      if (isKeyword(token, "FROM", "from")) {
        inFrom = true;
      } else if (inFrom && (isKeyword(token, "FILE", "file") || isKeyword(token, "RETENTION", "retention") ||
                            isKeyword(token, "VOLATILE", "volatile") || isKeyword(token, "STORAGE", "storage"))) {
        break;
      } else if (inFrom && previousSignificant != '.' && !isKeyword(token, "MIN", "min") && !isKeyword(token, "MAX", "max") &&
                 !isKeyword(token, "AVG", "avg") && !isKeyword(token, "SUMC", "sumc")) {
        retVal.push_back(token);
      }
      previousSignificant = 'I';
      continue;
    }

    if (std::isspace(uc) == 0) previousSignificant = c;
    ++i;
  }
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

  for (const auto &token : extractSourceStreams(query))
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
