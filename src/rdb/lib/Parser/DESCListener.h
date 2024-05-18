
// Generated from DESC.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "DESCParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by DESCParser.
 */
class  DESCListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterDesc(DESCParser::DescContext *ctx) = 0;
  virtual void exitDesc(DESCParser::DescContext *ctx) = 0;

  virtual void enterCommand(DESCParser::CommandContext *ctx) = 0;
  virtual void exitCommand(DESCParser::CommandContext *ctx) = 0;


};

