
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

  virtual void enterByteID(DESCParser::ByteIDContext *ctx) = 0;
  virtual void exitByteID(DESCParser::ByteIDContext *ctx) = 0;

  virtual void enterIntegerID(DESCParser::IntegerIDContext *ctx) = 0;
  virtual void exitIntegerID(DESCParser::IntegerIDContext *ctx) = 0;

  virtual void enterUnsignedID(DESCParser::UnsignedIDContext *ctx) = 0;
  virtual void exitUnsignedID(DESCParser::UnsignedIDContext *ctx) = 0;

  virtual void enterFloatID(DESCParser::FloatIDContext *ctx) = 0;
  virtual void exitFloatID(DESCParser::FloatIDContext *ctx) = 0;

  virtual void enterDoubleID(DESCParser::DoubleIDContext *ctx) = 0;
  virtual void exitDoubleID(DESCParser::DoubleIDContext *ctx) = 0;

  virtual void enterRationalID(DESCParser::RationalIDContext *ctx) = 0;
  virtual void exitRationalID(DESCParser::RationalIDContext *ctx) = 0;

  virtual void enterRefID(DESCParser::RefIDContext *ctx) = 0;
  virtual void exitRefID(DESCParser::RefIDContext *ctx) = 0;

  virtual void enterTypeID(DESCParser::TypeIDContext *ctx) = 0;
  virtual void exitTypeID(DESCParser::TypeIDContext *ctx) = 0;

  virtual void enterStringID(DESCParser::StringIDContext *ctx) = 0;
  virtual void exitStringID(DESCParser::StringIDContext *ctx) = 0;

  virtual void enterRetentionID(DESCParser::RetentionIDContext *ctx) = 0;
  virtual void exitRetentionID(DESCParser::RetentionIDContext *ctx) = 0;

  virtual void enterRetMemoryID(DESCParser::RetMemoryIDContext *ctx) = 0;
  virtual void exitRetMemoryID(DESCParser::RetMemoryIDContext *ctx) = 0;


};

