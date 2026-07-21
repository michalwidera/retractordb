#pragma once

#include <iostream>
#include <list>
#include <string>

#include <boost/rational.hpp>

#include "rdb/descriptor.hpp"

class token {
  command_id command_;
  rdb::descFldVT valueVT_;

 public:
  [[nodiscard]] std::string getStr_() const;
  [[nodiscard]] boost::rational<int> getRI() const;
  // S1: zwrot przez referencje — bylo przez wartosc, wiec kazdy odczyt kopiowal wariant
  // (a ten trzyma std::string / pair<string,int>). W goracej petli eval to alokacja per token.
  [[nodiscard]] constexpr const rdb::descFldVT &getVT() const { return valueVT_; };

  explicit token(command_id id = VOID_COMMAND, rdb::descFldVT value = 0);

  std::string getStrCommandID();
  [[nodiscard]] command_id getCommandID() const;

  friend std::ostream &operator<<(std::ostream &os, const token &rhs);
};
