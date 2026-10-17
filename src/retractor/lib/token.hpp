#pragma once

#include <iostream>
#include <list>
#include <string>

#include <boost/rational.hpp>

#include "rdb/descriptor.hpp"

/// Obiekt klasy token powinien:
/// - [token.token.1~1] przechowywać identyfikator polecenia command_id i wartość typu rdb::descFldVT, domyślnie VOID_COMMAND i 0.
/// - [token.token.2~1] udostępniać wartość przez getVT jako referencję bez kopiowania wariantu.
///   Sprawdzenie kompilacji: test/UnitTest/test_contracts.cpp - static_assert typu referencji zwracanej przez getVT().
/// - [token.token.3~1] przez getRI zwracać wartość przekonwertowaną na RATIONAL regułami cast.
/// - [token.token.4~1] przez getStr_ zwracać tekst wartości: null dla wartości pustej, napis bez zmian, liczbę dziesiętną dla BYTE i typów liczbowych, licznik/mianownik dla RATIONAL, parę rozdzieloną przecinkiem dla pary liczb oraz samą nazwę dla pary nazwa i indeks.
/// - [token.token.5~1] przez getCommandID i getStrCommandID udostępniać identyfikator polecenia i jego nazwę.
/// - [token.token.6~1] wypisywać token w postaci diagnostycznej <nazwa polecenia>(<wartość>), w której para nazwa i indeks ma postać nazwa[indeks].
class token {
  command_id command_;
  rdb::descFldVT valueVT_;

 public:
  [[nodiscard]] std::string getStr_() const;
  [[nodiscard]] boost::rational<int> getRI() const;
  // S1: zwrot przez referencje - bylo przez wartosc, wiec kazdy odczyt kopiowal wariant
  // (a ten trzyma std::string / pair<string,int>). W goracej petli eval to alokacja per token.
  [[nodiscard]] constexpr const rdb::descFldVT &getVT() const { return valueVT_; };

  explicit token(command_id id = VOID_COMMAND, rdb::descFldVT value = 0);

  std::string getStrCommandID();
  [[nodiscard]] command_id getCommandID() const;

  friend std::ostream &operator<<(std::ostream &os, const token &rhs);
};
