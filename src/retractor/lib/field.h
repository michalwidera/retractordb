#pragma once

#include <list>

#include "token.h"
#include "rdb/descriptor.h"

struct field {
  std::list<token> lProgram;

  rdb::rField field_;

  explicit field(rdb::rField field_, token lToken);
  explicit field(rdb::rField field_, std::list<token> lProgram);

  token getFirstFieldToken();

  friend std::ostream &operator<<(std::ostream &os, const rdb::descFld &s);
  friend std::ostream &operator<<(std::ostream &os, const field &s);
};
