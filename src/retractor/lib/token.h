#pragma once

#include <iostream>
#include <list>
#include <string>

#include <boost/rational.hpp>

#include "rdb/descriptor.h"

class token {
  command_id command_;
  rdb::descFldVT valueVT_;

 public:
  std::string getStr_();
  boost::rational<int> getRI();
  constexpr rdb::descFldVT getVT() const { return valueVT_; };

  explicit token(command_id id = VOID_COMMAND, rdb::descFldVT value = 0);

  std::string getStrCommandID();
  command_id getCommandID();

  friend std::ostream &operator<<(std::ostream &os, const token &rhs);
};
