
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

  virtual void enterByte(DESCParser::ByteContext *ctx) = 0;
  virtual void exitByte(DESCParser::ByteContext *ctx) = 0;

  virtual void enterString(DESCParser::StringContext *ctx) = 0;
  virtual void exitString(DESCParser::StringContext *ctx) = 0;

  virtual void enterUnsigned(DESCParser::UnsignedContext *ctx) = 0;
  virtual void exitUnsigned(DESCParser::UnsignedContext *ctx) = 0;

  virtual void enterFloat(DESCParser::FloatContext *ctx) = 0;
  virtual void exitFloat(DESCParser::FloatContext *ctx) = 0;

  virtual void enterDouble(DESCParser::DoubleContext *ctx) = 0;
  virtual void exitDouble(DESCParser::DoubleContext *ctx) = 0;

  virtual void enterRef(DESCParser::RefContext *ctx) = 0;
  virtual void exitRef(DESCParser::RefContext *ctx) = 0;

  virtual void enterType(DESCParser::TypeContext *ctx) = 0;
  virtual void exitType(DESCParser::TypeContext *ctx) = 0;


};

