
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

  virtual void enterByteID(DESCParser::ByteIDContext * /*ctx*/) override { }
  virtual void exitByteID(DESCParser::ByteIDContext * /*ctx*/) override { }

  virtual void enterIntegerID(DESCParser::IntegerIDContext * /*ctx*/) override { }
  virtual void exitIntegerID(DESCParser::IntegerIDContext * /*ctx*/) override { }

  virtual void enterUnsignedID(DESCParser::UnsignedIDContext * /*ctx*/) override { }
  virtual void exitUnsignedID(DESCParser::UnsignedIDContext * /*ctx*/) override { }

  virtual void enterFloatID(DESCParser::FloatIDContext * /*ctx*/) override { }
  virtual void exitFloatID(DESCParser::FloatIDContext * /*ctx*/) override { }

  virtual void enterDoubleID(DESCParser::DoubleIDContext * /*ctx*/) override { }
  virtual void exitDoubleID(DESCParser::DoubleIDContext * /*ctx*/) override { }

  virtual void enterRefID(DESCParser::RefIDContext * /*ctx*/) override { }
  virtual void exitRefID(DESCParser::RefIDContext * /*ctx*/) override { }

  virtual void enterTypeID(DESCParser::TypeIDContext * /*ctx*/) override { }
  virtual void exitTypeID(DESCParser::TypeIDContext * /*ctx*/) override { }

  virtual void enterStringID(DESCParser::StringIDContext * /*ctx*/) override { }
  virtual void exitStringID(DESCParser::StringIDContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

