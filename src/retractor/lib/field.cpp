#include "field.h"

#include <cassert>

/** Construktor set */

field::field(rdb::rField field_, token lToken)
    :                            //
      field_(std::move(field_))  //
{
  lProgram.push_back(lToken);
}

field::field(rdb::rField field_, std::list<token> lProgram)
    :                      //
      lProgram(lProgram),  //
      field_(std::move(field_)) {}

token field::getFirstFieldToken() {
  assert(lProgram.size() > 0);  // If this fails that means in field no program (decl!)
  return *lProgram.begin();
}

std::ostream &operator<<(std::ostream &os, rdb::descFld &s) { return os << rdb::GetStringdescFld(s); }

std::ostream &operator<<(std::ostream &os, const field &s) {
  os << "FLD/";
  os << "name:" << s.field_.rname << ",";
  os << "type:" << s.field_.rtype << ",";
  os << "prog:";
  for (auto &i : s.lProgram)
    os << i << ",";
  return os;
}
