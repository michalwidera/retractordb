#include "field.hpp"

#include <utility>

#include "fatalError.hpp"

/** Construktor set */

field::field(rdb::rField field_, const token &lToken)
    :                            //
      field_(std::move(field_))  //
{
  lProgram.push_back(lToken);
}

field::field(rdb::rField field_, std::list<token> lProgram)
    : lProgram(std::move(lProgram)),  //
      field_(std::move(field_)) {}

token field::getFirstFieldToken() {
  if (lProgram.empty()) FatalError("field::getFirstFieldToken: no program tokens — should not be called on a declaration field");
  return *lProgram.begin();
}

std::ostream &operator<<(std::ostream &os, rdb::descFld &s) { return os << rdb::GetStringdescFld(s); }

std::ostream &operator<<(std::ostream &os, const field &s) {
  os << "FLD/";
  os << "name:" << s.field_.rname << ",";
  os << "type:" << static_cast<int>(s.field_.rtype) << ",";
  os << "prog:";
  for (const auto &i : s.lProgram)
    os << i << ",";
  return os;
}
