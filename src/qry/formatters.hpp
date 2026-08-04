#pragma once
#include <deque>
#include <string>
#include <tuple>
#include <vector>

#include <boost/property_tree/ptree.hpp>

enum class formatMode : std::uint8_t { RAW, GRAPHITE, INFLUXDB, GNUPLOT };

class Formatter {
  std::vector<std::deque<std::string>> gnuplot_lines_;
  static const std::vector<std::string> colors_;

  /// @brief Ile POCZATKOWYCH rekordow strumienia pominac, zanim zacznie sie zbieranie do wykresu.
  ///
  /// Strumien wyliczany zaczyna sie tam, gdzie jego definicja staje sie kompletna
  /// (query::logicalOrigin), ale jego pierwsze rekordy odpowiadaja na probki LEZACE PRZED tym
  /// poczatkiem — bo okna i przesuniecia siegaja w historie zrodla, ktora do strumienia juz nie
  /// trafia. Na wykresie widac wtedy odpowiedz bez widocznej przyczyny: w potoku QRS
  /// (examples/ecg) pierwszy impuls detekcji nalezy do zespolu, ktorego szczyt wypada przed
  /// poczatkiem strumienia. To nie jest naruszenie przyczynowosci, tylko uciety kadr — ale
  /// wyglada dokladnie jak „odpowiedz przed danymi" i zadaje falszywe pytanie.
  int gnuplot_warmup_{0};
  int gnuplot_skipped_{0};

 public:
  /// Domyslnie zero — pomijanie wlacza swiadomie warstwa uruchamiajaca (xqry).
  void setGnuplotWarmup(int records) { gnuplot_warmup_ = records > 0 ? records : 0; }

  static bool isNullAt(const std::string &nullmap, int index);
  static bool isAllNull(const std::string &nullmap, int count);
  static std::string displayedValue(const boost::property_tree::ptree &row, int index, const std::string &nullmap,
                                    formatMode mode);

  static void initGnuplot(std::tuple<int, int, int> dim, bool rightToLeft);
  static void renderRaw(const boost::property_tree::ptree &row, int count, const std::string &nullmap, bool skipNull);
  void renderGnuplot(const boost::property_tree::ptree &row, int count, const std::string &nullmap, const std::string &input,
                     const boost::property_tree::ptree &schema, std::tuple<int, int, int> dim);
  static void renderGraphite(const boost::property_tree::ptree &row, const std::string &nullmap, const std::string &input,
                             const boost::property_tree::ptree &schema);
  static void renderInfluxDB(const boost::property_tree::ptree &row, const std::string &nullmap, const std::string &input,
                             const boost::property_tree::ptree &schema);
};
