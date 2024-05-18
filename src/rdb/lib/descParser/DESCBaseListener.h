
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

  virtual void enterCommand(DESCParser::CommandContext * /*ctx*/) override { }
  virtual void exitCommand(DESCParser::CommandContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

