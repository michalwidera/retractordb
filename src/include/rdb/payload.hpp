#pragma once

#include <any>
#include <optional>
#include <span>
#include <vector>

#include "descriptor.hpp"

/// @brief Definicja klasy zarządzającej obszarem pamięci interpretowanym zgodnie z opisem w Descriptor.
///
/// Obiekt klasy payload powinien:
/// - zarządzać binarnym obszarem pamięci odpowiadającym układowi pól opisanemu przez Descriptor,
/// - umożliwiać ustawianie i pobieranie wartości pól na podstawie pozycji w spłaszczonym widoku Descriptor,
/// - obsługiwać pola danych BYTE, INTEGER, UINT, RATIONAL, FLOAT, DOUBLE, STRING i NULLTYPE; dla pola bez
///   znacznika null zwracać z getItem() std::any zawierające odpowiednio uint8_t, int, unsigned int,
///   boost::rational<int>, float, double, std::string albo std::monostate; dla RATIONAL z mianownikiem
///   niedodatnim w buforze zwracać std::nullopt,
/// - w setItem() konwertować niepustą wartość przez cast<std::any>(wartość, typ_pola) z convertTypes.hpp;
///   wynik std::monostate dla pola innego niż NULLTYPE zamieniać na znacznik null i nullFallbackValue(typ_pola),
///   a po udanej konwersji kasować znacznik null; setItem(..., std::nullopt) ustawia znacznik null i zapisuje
///   nullFallbackValue(typ_pola): zero liczbowe, 0/1 dla RATIONAL, pusty STRING albo std::monostate dla NULLTYPE,
/// - przy konwersji liczby do BYTE, INTEGER lub UINT sprawdzać zakres typu docelowego po obcięciu części
///   ułamkowej ku zeru; wartość poza zakresem oraz NaN i nieskończoność zamieniać na null; przy konwersji
///   STRING do tych typów odczytywać przez std::from_chars prefiks typu int, a do FLOAT/DOUBLE prefiks
///   typu double, bez wymogu odczytania całego napisu, następnie konwertować do typu pola,
/// - zapisywać STRING w najwyżej rlen * rarray bajtach, przycinając dłuższą wartość i dopełniając krótszą
///   zerami; getItem() zwraca tekst do pierwszego bajtu zerowego albo do końca pola, jeśli nie ma zera,
/// - zwalniać własną pamięć przy niszczeniu obiektu,
/// - udostępniać tekstową reprezentację danych przez operator<< oraz wczytywanie wartości przez operator>>,
/// - umożliwiać formatowanie tekstowej reprezentacji wartości numerycznych w trybie dziesiętnym lub szesnastkowym,
/// - wspierać kopiowanie stanu obiektu przez konstruktor kopiujący i operator=,
/// - wspierać przenoszenie stanu obiektu przez konstruktor przenoszący i operator=(payload&&), zachowując regułę zgodności deskryptorów obowiązującą przy przypisaniu,
/// - wspierać łączenie danych dwóch obiektów payload przez operator+,
/// - dla pola oznaczonego jako null w nullBitset zwracać z getItem() std::nullopt; dla pola NULLTYPE
///   bez tego oznaczenia zwracać std::any zawierające std::monostate; w obu przypadkach operator<< wypisuje "null".

namespace rdb {
/// @brief This class define accessing method to payload (memory area)
/// This is accessor for payload memory area that supports applying descriptor type over memory area.
class payload {
  /// @brief Payload memory area
  std::vector<uint8_t> payloadData_;

  /// @brief Type of dumped or read numeric formats
  bool hexFormat_ = false;

  /// @return false, gdy @p value nie niesie typu T - wolajacy loguje i pomija zapis pola.
  template <typename T>
  [[nodiscard]] bool setItemBy(int position, const std::any &value);

  payload &operator=(const Descriptor &other);

  /// @brief Przenosi znaczniki NULL ze zgodnego payloadu o innej liczbie wpisow deskryptora
  void retargetNullBitsetFrom(const payload &other);

  std::vector<bool> nullBitset_;  // true if field at position i is null, false otherwise
 public:
  /// @brief Descriptor of managed payload area
  Descriptor descriptor;

  /// @brief Span accessor to payload (modern, bounds-aware)
  /// @return  std::span over the payload memory
  std::span<uint8_t> span();
  [[nodiscard]] std::span<const uint8_t> span() const;

  /// @brief Constructor of payload object
  /// @param descriptor descriptor of payload area
  explicit payload(const Descriptor &descriptor);

  /// @brief Copy constructor
  /// @param from destructor
  payload(const payload &other);

  /// @brief Konstruktor przenoszący - przejmuje deskryptor, bufor i znaczniki NULL źródła.
  ///
  /// Źródło zostaje odpowiednikiem obiektu z konstruktora domyślnego (deskryptor pusty,
  /// span() pusty), więc nadaje się do zniszczenia i do ponownego przypisania.
  /// Formatu hex NIE przejmuje - tak samo jak konstruktor kopiujący, który zostawia
  /// wartość domyślną; przenoszenie ma być obserwacyjnie równe kopiowaniu.
  payload(payload &&other) noexcept;

  /// @brief Default constructor is allowed - creates uninitialized object, ready to assign
  payload() = default;

  /// @brief Direct setter
  /// @param position position according to descriptor
  /// @param valueParam value of given type according to descriptor that will be set
  /// in payload
  void setItem(int position, std::optional<std::any> valueParam);

  /// @brief Direct getter
  /// @param position position according to descriptor
  /// @return address of beginning memory that contains data described by
  /// descriptor
  [[nodiscard]] std::optional<std::any> getItem(int position) const;

  /// @brief Wariantowy setter (P1, speed_improvement): pisze descFldVT wprost do
  /// bajtow, bez posrednika std::any (eliminuje konwersje any<->wariant na
  /// goracej sciezce). Semantyka identyczna z setItem (nullopt => null).
  void setItemVT(int position, std::optional<descFldVT> valueParam);

  /// @brief Wariantowy getter (P1): czyta bajty do descFldVT bez std::any.
  /// Semantyka identyczna z getItem (nullopt => null).
  [[nodiscard]] std::optional<descFldVT> getItemVT(int position) const;

  /// @brief Odczyt pola BYTE/INTEGER wprost do int - bez wariantu, dla goracych petli, ktore znaja
  /// typ z gory (redukcja pol rekordu). Semantyka NULL identyczna z getItemVT (nullopt => null).
  /// Pole innego typu to blad wolajacego (rdb::fatal - kompilator sprawdzil typ wczesniej).
  [[nodiscard]] std::optional<int> getIntegralItem(int position) const;

  /// @brief Set format input/output formater - default false
  /// @param hexFormat true if out/in in hex
  void setHex(bool hexFormat);

  /// @brief Expose null metadata for external persistence (metaData)
  [[nodiscard]] const std::vector<bool> &getNullBitset() const;

  /// @brief Restore null metadata read from metaData/text source
  void setNullBitset(const std::vector<bool> &nullBitset);

  friend std::istream &operator>>(std::istream &is, payload &rhs);
  friend std::ostream &operator<<(std::ostream &os, const payload &rhs);

  payload &operator=(const payload &other);

  /// @brief Przypisanie przenoszące - ta sama reguła zgodności deskryptorów co przy przypisaniu kopiującym.
  ///
  /// To NIE jest domyślna semantyka przenoszenia: kradzież całego stanu źródła zmieniłaby
  /// zachowanie silnika. Reguła jest ta sama co przy operator=(const Descriptor&):
  /// deskryptor pusty - cel przejmuje cudzy wraz z buforem i znacznikami NULL; deskryptor
  /// zgodny - cel zachowuje WŁASNY deskryptor, a stan idzie dokładnie drogą kopii, bez
  /// przejmowania czegokolwiek; deskryptor niezgodny - złamany niezmiennik (RDB_ASSERT). Dlaczego
  /// zgodny cel nie może niczego ukraść - przy definicji w payload.cc.
  payload &operator=(payload &&other) noexcept;

  payload operator+(const payload &other) const;

  /// @brief Zapisuje do tego payloadu sume `lhs + rhs` bez budowy payloadu i deskryptora sumy.
  ///
  /// Wynik jest taki sam jak `*this = lhs + rhs`, ale tylko gdy uklad tego payloadu jest dokladnie
  /// ukladem sumy: deskryptor zgodny z deskryptorem sumy w obie strony i o tej samej liczbie wpisow.
  /// Sprawdza to wolajacy, raz na wezel (dataModel::constructInputPayload) - tu nie ma tego warunku.
  void assignConcatenation(const payload &lhs, const payload &rhs);
};
}  // namespace rdb
