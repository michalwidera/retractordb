
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "RQLParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by RQLParser.
 */
class  RQLListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProg(RQLParser::ProgContext *ctx) = 0;
  virtual void exitProg(RQLParser::ProgContext *ctx) = 0;

  virtual void enterSelect(RQLParser::SelectContext *ctx) = 0;
  virtual void exitSelect(RQLParser::SelectContext *ctx) = 0;

  virtual void enterRationalAsFraction(RQLParser::RationalAsFractionContext *ctx) = 0;
  virtual void exitRationalAsFraction(RQLParser::RationalAsFractionContext *ctx) = 0;

  virtual void enterRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) = 0;
  virtual void exitRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) = 0;

  virtual void enterRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) = 0;
  virtual void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) = 0;

  virtual void enterFraction(RQLParser::FractionContext *ctx) = 0;
  virtual void exitFraction(RQLParser::FractionContext *ctx) = 0;

  virtual void enterDeclare(RQLParser::DeclareContext *ctx) = 0;
  virtual void exitDeclare(RQLParser::DeclareContext *ctx) = 0;

  virtual void enterDeclarationList(RQLParser::DeclarationListContext *ctx) = 0;
  virtual void exitDeclarationList(RQLParser::DeclarationListContext *ctx) = 0;

  virtual void enterSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) = 0;
  virtual void exitSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) = 0;

  virtual void enterTypeArray(RQLParser::TypeArrayContext *ctx) = 0;
  virtual void exitTypeArray(RQLParser::TypeArrayContext *ctx) = 0;

  virtual void enterTypeByte(RQLParser::TypeByteContext *ctx) = 0;
  virtual void exitTypeByte(RQLParser::TypeByteContext *ctx) = 0;

  virtual void enterTypeInt(RQLParser::TypeIntContext *ctx) = 0;
  virtual void exitTypeInt(RQLParser::TypeIntContext *ctx) = 0;

  virtual void enterTypeUnsiged(RQLParser::TypeUnsigedContext *ctx) = 0;
  virtual void exitTypeUnsiged(RQLParser::TypeUnsigedContext *ctx) = 0;

  virtual void enterTypeFloat(RQLParser::TypeFloatContext *ctx) = 0;
  virtual void exitTypeFloat(RQLParser::TypeFloatContext *ctx) = 0;

  virtual void enterSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) = 0;
  virtual void exitSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) = 0;

  virtual void enterSelectList(RQLParser::SelectListContext *ctx) = 0;
  virtual void exitSelectList(RQLParser::SelectListContext *ctx) = 0;

  virtual void enterFieldID(RQLParser::FieldIDContext *ctx) = 0;
  virtual void exitFieldID(RQLParser::FieldIDContext *ctx) = 0;

  virtual void enterFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) = 0;
  virtual void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) = 0;

  virtual void enterFieldIDColumnname(RQLParser::FieldIDColumnnameContext *ctx) = 0;
  virtual void exitFieldIDColumnname(RQLParser::FieldIDColumnnameContext *ctx) = 0;

  virtual void enterFieldIDTable(RQLParser::FieldIDTableContext *ctx) = 0;
  virtual void exitFieldIDTable(RQLParser::FieldIDTableContext *ctx) = 0;

  virtual void enterUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) = 0;
  virtual void exitUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) = 0;

  virtual void enterAsterisk(RQLParser::AsteriskContext *ctx) = 0;
  virtual void exitAsterisk(RQLParser::AsteriskContext *ctx) = 0;

  virtual void enterExpression(RQLParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(RQLParser::ExpressionContext *ctx) = 0;

  virtual void enterExpPlus(RQLParser::ExpPlusContext *ctx) = 0;
  virtual void exitExpPlus(RQLParser::ExpPlusContext *ctx) = 0;

  virtual void enterExpMinus(RQLParser::ExpMinusContext *ctx) = 0;
  virtual void exitExpMinus(RQLParser::ExpMinusContext *ctx) = 0;

  virtual void enterExpTerm(RQLParser::ExpTermContext *ctx) = 0;
  virtual void exitExpTerm(RQLParser::ExpTermContext *ctx) = 0;

  virtual void enterExpIn(RQLParser::ExpInContext *ctx) = 0;
  virtual void exitExpIn(RQLParser::ExpInContext *ctx) = 0;

  virtual void enterExpDiv(RQLParser::ExpDivContext *ctx) = 0;
  virtual void exitExpDiv(RQLParser::ExpDivContext *ctx) = 0;

  virtual void enterExpFactor(RQLParser::ExpFactorContext *ctx) = 0;
  virtual void exitExpFactor(RQLParser::ExpFactorContext *ctx) = 0;

  virtual void enterExpMult(RQLParser::ExpMultContext *ctx) = 0;
  virtual void exitExpMult(RQLParser::ExpMultContext *ctx) = 0;

  virtual void enterExpFloat(RQLParser::ExpFloatContext *ctx) = 0;
  virtual void exitExpFloat(RQLParser::ExpFloatContext *ctx) = 0;

  virtual void enterExpDec(RQLParser::ExpDecContext *ctx) = 0;
  virtual void exitExpDec(RQLParser::ExpDecContext *ctx) = 0;

  virtual void enterExpUnary(RQLParser::ExpUnaryContext *ctx) = 0;
  virtual void exitExpUnary(RQLParser::ExpUnaryContext *ctx) = 0;

  virtual void enterExpFnCall(RQLParser::ExpFnCallContext *ctx) = 0;
  virtual void exitExpFnCall(RQLParser::ExpFnCallContext *ctx) = 0;

  virtual void enterExpField(RQLParser::ExpFieldContext *ctx) = 0;
  virtual void exitExpField(RQLParser::ExpFieldContext *ctx) = 0;

  virtual void enterExpAgg(RQLParser::ExpAggContext *ctx) = 0;
  virtual void exitExpAgg(RQLParser::ExpAggContext *ctx) = 0;

  virtual void enterSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) = 0;
  virtual void exitSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) = 0;

  virtual void enterSExpPlus(RQLParser::SExpPlusContext *ctx) = 0;
  virtual void exitSExpPlus(RQLParser::SExpPlusContext *ctx) = 0;

  virtual void enterSExpMinus(RQLParser::SExpMinusContext *ctx) = 0;
  virtual void exitSExpMinus(RQLParser::SExpMinusContext *ctx) = 0;

  virtual void enterSExpTerm(RQLParser::SExpTermContext *ctx) = 0;
  virtual void exitSExpTerm(RQLParser::SExpTermContext *ctx) = 0;

  virtual void enterSExpHash(RQLParser::SExpHashContext *ctx) = 0;
  virtual void exitSExpHash(RQLParser::SExpHashContext *ctx) = 0;

  virtual void enterSExpAnd(RQLParser::SExpAndContext *ctx) = 0;
  virtual void exitSExpAnd(RQLParser::SExpAndContext *ctx) = 0;

  virtual void enterSExpMod(RQLParser::SExpModContext *ctx) = 0;
  virtual void exitSExpMod(RQLParser::SExpModContext *ctx) = 0;

  virtual void enterSExpAgse(RQLParser::SExpAgseContext *ctx) = 0;
  virtual void exitSExpAgse(RQLParser::SExpAgseContext *ctx) = 0;

  virtual void enterSExpAgregate(RQLParser::SExpAgregateContext *ctx) = 0;
  virtual void exitSExpAgregate(RQLParser::SExpAgregateContext *ctx) = 0;

  virtual void enterSExpFactor(RQLParser::SExpFactorContext *ctx) = 0;
  virtual void exitSExpFactor(RQLParser::SExpFactorContext *ctx) = 0;

  virtual void enterStream_factor(RQLParser::Stream_factorContext *ctx) = 0;
  virtual void exitStream_factor(RQLParser::Stream_factorContext *ctx) = 0;

  virtual void enterStreamMin(RQLParser::StreamMinContext *ctx) = 0;
  virtual void exitStreamMin(RQLParser::StreamMinContext *ctx) = 0;

  virtual void enterStreamMax(RQLParser::StreamMaxContext *ctx) = 0;
  virtual void exitStreamMax(RQLParser::StreamMaxContext *ctx) = 0;

  virtual void enterStreamAvg(RQLParser::StreamAvgContext *ctx) = 0;
  virtual void exitStreamAvg(RQLParser::StreamAvgContext *ctx) = 0;

  virtual void enterStreamSum(RQLParser::StreamSumContext *ctx) = 0;
  virtual void exitStreamSum(RQLParser::StreamSumContext *ctx) = 0;

  virtual void enterFunction_call(RQLParser::Function_callContext *ctx) = 0;
  virtual void exitFunction_call(RQLParser::Function_callContext *ctx) = 0;


};

