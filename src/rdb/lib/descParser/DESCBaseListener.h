
// Generated from DESC.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "DESCListener.h"


/**
 * This class provides an empty implementation of DESCListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  DESCBaseListener : public DESCListener {
public:

  virtual void enterDesc(DESCParser::DescContext * /*ctx*/) override { }
  virtual void exitDesc(DESCParser::DescContext * /*ctx*/) override { }

  virtual void enterByte(DESCParser::ByteContext * /*ctx*/) override { }
  virtual void exitByte(DESCParser::ByteContext * /*ctx*/) override { }

  virtual void enterString(DESCParser::StringContext * /*ctx*/) override { }
  virtual void exitString(DESCParser::StringContext * /*ctx*/) override { }

  virtual void enterUnsigned(DESCParser::UnsignedContext * /*ctx*/) override { }
  virtual void exitUnsigned(DESCParser::UnsignedContext * /*ctx*/) override { }

  virtual void enterFloat(DESCParser::FloatContext * /*ctx*/) override { }
  virtual void exitFloat(DESCParser::FloatContext * /*ctx*/) override { }

  virtual void enterDouble(DESCParser::DoubleContext * /*ctx*/) override { }
  virtual void exitDouble(DESCParser::DoubleContext * /*ctx*/) override { }

  virtual void enterRef(DESCParser::RefContext * /*ctx*/) override { }
  virtual void exitRef(DESCParser::RefContext * /*ctx*/) override { }

  virtual void enterType(DESCParser::TypeContext * /*ctx*/) override { }
  virtual void exitType(DESCParser::TypeContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

