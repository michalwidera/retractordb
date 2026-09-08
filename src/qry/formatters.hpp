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

 public:
  /// Jedna WARTOSC wiersza: pole, ktore ja niesie, jej pozycja w tym polu i krotnosc pola.
  /// Serwer splaszcza tablice (jedna wartosc na element, nie na pole), wiec sama lista pol
  /// ze schematu przestala opisywac wiersz -- kolumny sa jej rozwinieciem.
  struct Column {
    std::string field;
    int index;
    int width;
  };

  /// Splaszczone kolumny odpowiedzi 'detail'. Krotnosc pola przychodzi w
  /// db.field_count.<pole>; starszy serwer jej nie wysyla i wtedy pole liczy sie jako
  /// jednoelementowe -- klient nadal dziala, tylko nie rozwija tablic.
  static std::vector<Column> columns(const boost::property_tree::ptree &schema);

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
