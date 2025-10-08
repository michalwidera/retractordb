
// Generated from RQL.g4 by ANTLR 4.13.1

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

  virtual void enterCoption(RQLParser::CoptionContext *ctx) = 0;
  virtual void exitCoption(RQLParser::CoptionContext *ctx) = 0;

  virtual void enterStorage(RQLParser::StorageContext *ctx) = 0;
  virtual void exitStorage(RQLParser::StorageContext *ctx) = 0;

  virtual void enterSubstrat(RQLParser::SubstratContext *ctx) = 0;
  virtual void exitSubstrat(RQLParser::SubstratContext *ctx) = 0;

  virtual void enterSelect(RQLParser::SelectContext *ctx) = 0;
  virtual void exitSelect(RQLParser::SelectContext *ctx) = 0;

  virtual void enterDeclare(RQLParser::DeclareContext *ctx) = 0;
  virtual void exitDeclare(RQLParser::DeclareContext *ctx) = 0;

  virtual void enterRulez(RQLParser::RulezContext *ctx) = 0;
  virtual void exitRulez(RQLParser::RulezContext *ctx) = 0;

  virtual void enterDumppart(RQLParser::DumppartContext *ctx) = 0;
  virtual void exitDumppart(RQLParser::DumppartContext *ctx) = 0;

  virtual void enterSystempart(RQLParser::SystempartContext *ctx) = 0;
  virtual void exitSystempart(RQLParser::SystempartContext *ctx) = 0;

  virtual void enterRationalAsFraction_proforma(RQLParser::RationalAsFraction_proformaContext *ctx) = 0;
  virtual void exitRationalAsFraction_proforma(RQLParser::RationalAsFraction_proformaContext *ctx) = 0;

  virtual void enterRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) = 0;
  virtual void exitRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) = 0;

  virtual void enterRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) = 0;
  virtual void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) = 0;

  virtual void enterRetention(RQLParser::RetentionContext *ctx) = 0;
  virtual void exitRetention(RQLParser::RetentionContext *ctx) = 0;

  virtual void enterFraction(RQLParser::FractionContext *ctx) = 0;
  virtual void exitFraction(RQLParser::FractionContext *ctx) = 0;

  virtual void enterSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) = 0;
  virtual void exitSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) = 0;

  virtual void enterTypeByte(RQLParser::TypeByteContext *ctx) = 0;
  virtual void exitTypeByte(RQLParser::TypeByteContext *ctx) = 0;

  virtual void enterTypeInt(RQLParser::TypeIntContext *ctx) = 0;
  virtual void exitTypeInt(RQLParser::TypeIntContext *ctx) = 0;

  virtual void enterTypeUnsigned(RQLParser::TypeUnsignedContext *ctx) = 0;
  virtual void exitTypeUnsigned(RQLParser::TypeUnsignedContext *ctx) = 0;

  virtual void enterTypeFloat(RQLParser::TypeFloatContext *ctx) = 0;
  virtual void exitTypeFloat(RQLParser::TypeFloatContext *ctx) = 0;

  virtual void enterTypeDouble(RQLParser::TypeDoubleContext *ctx) = 0;
  virtual void exitTypeDouble(RQLParser::TypeDoubleContext *ctx) = 0;

  virtual void enterTypeString(RQLParser::TypeStringContext *ctx) = 0;
  virtual void exitTypeString(RQLParser::TypeStringContext *ctx) = 0;

  virtual void enterSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) = 0;
  virtual void exitSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) = 0;

  virtual void enterSelectList(RQLParser::SelectListContext *ctx) = 0;
  virtual void exitSelectList(RQLParser::SelectListContext *ctx) = 0;

  virtual void enterFieldID(RQLParser::FieldIDContext *ctx) = 0;
  virtual void exitFieldID(RQLParser::FieldIDContext *ctx) = 0;

  virtual void enterFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) = 0;
  virtual void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) = 0;

  virtual void enterFieldIDColumnName(RQLParser::FieldIDColumnNameContext *ctx) = 0;
  virtual void exitFieldIDColumnName(RQLParser::FieldIDColumnNameContext *ctx) = 0;

  virtual void enterFieldIDTable(RQLParser::FieldIDTableContext *ctx) = 0;
  virtual void exitFieldIDTable(RQLParser::FieldIDTableContext *ctx) = 0;

  virtual void enterUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) = 0;
  virtual void exitUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) = 0;

  virtual void enterAsterisk(RQLParser::AsteriskContext *ctx) = 0;
  virtual void exitAsterisk(RQLParser::AsteriskContext *ctx) = 0;

  virtual void enterExpression(RQLParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(RQLParser::ExpressionContext *ctx) = 0;

  virtual void enterLogicExpression(RQLParser::LogicExpressionContext *ctx) = 0;
  virtual void exitLogicExpression(RQLParser::LogicExpressionContext *ctx) = 0;

  virtual void enterExpOr(RQLParser::ExpOrContext *ctx) = 0;
  virtual void exitExpOr(RQLParser::ExpOrContext *ctx) = 0;

  virtual void enterExpAnd(RQLParser::ExpAndContext *ctx) = 0;
  virtual void exitExpAnd(RQLParser::ExpAndContext *ctx) = 0;

  virtual void enterExpTermLogic(RQLParser::ExpTermLogicContext *ctx) = 0;
  virtual void exitExpTermLogic(RQLParser::ExpTermLogicContext *ctx) = 0;

  virtual void enterExpLs(RQLParser::ExpLsContext *ctx) = 0;
  virtual void exitExpLs(RQLParser::ExpLsContext *ctx) = 0;

  virtual void enterExpLe(RQLParser::ExpLeContext *ctx) = 0;
  virtual void exitExpLe(RQLParser::ExpLeContext *ctx) = 0;

  virtual void enterExpNq(RQLParser::ExpNqContext *ctx) = 0;
  virtual void exitExpNq(RQLParser::ExpNqContext *ctx) = 0;

  virtual void enterExpGr(RQLParser::ExpGrContext *ctx) = 0;
  virtual void exitExpGr(RQLParser::ExpGrContext *ctx) = 0;

  virtual void enterExpFactor(RQLParser::ExpFactorContext *ctx) = 0;
  virtual void exitExpFactor(RQLParser::ExpFactorContext *ctx) = 0;

  virtual void enterExpEq(RQLParser::ExpEqContext *ctx) = 0;
  virtual void exitExpEq(RQLParser::ExpEqContext *ctx) = 0;

  virtual void enterExpGe(RQLParser::ExpGeContext *ctx) = 0;
  virtual void exitExpGe(RQLParser::ExpGeContext *ctx) = 0;

  virtual void enterExpPlus(RQLParser::ExpPlusContext *ctx) = 0;
  virtual void exitExpPlus(RQLParser::ExpPlusContext *ctx) = 0;

  virtual void enterExpMinus(RQLParser::ExpMinusContext *ctx) = 0;
  virtual void exitExpMinus(RQLParser::ExpMinusContext *ctx) = 0;

  virtual void enterExpTerm(RQLParser::ExpTermContext *ctx) = 0;
  virtual void exitExpTerm(RQLParser::ExpTermContext *ctx) = 0;

  virtual void enterExpIn(RQLParser::ExpInContext *ctx) = 0;
  virtual void exitExpIn(RQLParser::ExpInContext *ctx) = 0;

  virtual void enterExpRational(RQLParser::ExpRationalContext *ctx) = 0;
  virtual void exitExpRational(RQLParser::ExpRationalContext *ctx) = 0;

  virtual void enterExpFloat(RQLParser::ExpFloatContext *ctx) = 0;
  virtual void exitExpFloat(RQLParser::ExpFloatContext *ctx) = 0;

  virtual void enterExpDec(RQLParser::ExpDecContext *ctx) = 0;
  virtual void exitExpDec(RQLParser::ExpDecContext *ctx) = 0;

  virtual void enterExpAgg(RQLParser::ExpAggContext *ctx) = 0;
  virtual void exitExpAgg(RQLParser::ExpAggContext *ctx) = 0;

  virtual void enterExpFnCall(RQLParser::ExpFnCallContext *ctx) = 0;
  virtual void exitExpFnCall(RQLParser::ExpFnCallContext *ctx) = 0;

  virtual void enterExpDiv(RQLParser::ExpDivContext *ctx) = 0;
  virtual void exitExpDiv(RQLParser::ExpDivContext *ctx) = 0;

  virtual void enterExpField(RQLParser::ExpFieldContext *ctx) = 0;
  virtual void exitExpField(RQLParser::ExpFieldContext *ctx) = 0;

  virtual void enterExpNot(RQLParser::ExpNotContext *ctx) = 0;
  virtual void exitExpNot(RQLParser::ExpNotContext *ctx) = 0;

  virtual void enterExpString(RQLParser::ExpStringContext *ctx) = 0;
  virtual void exitExpString(RQLParser::ExpStringContext *ctx) = 0;

  virtual void enterExpUnary(RQLParser::ExpUnaryContext *ctx) = 0;
  virtual void exitExpUnary(RQLParser::ExpUnaryContext *ctx) = 0;

  virtual void enterExpMult(RQLParser::ExpMultContext *ctx) = 0;
  virtual void exitExpMult(RQLParser::ExpMultContext *ctx) = 0;

  virtual void enterSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) = 0;
  virtual void exitSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) = 0;

  virtual void enterSExpMinus(RQLParser::SExpMinusContext *ctx) = 0;
  virtual void exitSExpMinus(RQLParser::SExpMinusContext *ctx) = 0;

  virtual void enterSExpPlus(RQLParser::SExpPlusContext *ctx) = 0;
  virtual void exitSExpPlus(RQLParser::SExpPlusContext *ctx) = 0;

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

  virtual void enterSExpAgregate_proforma(RQLParser::SExpAgregate_proformaContext *ctx) = 0;
  virtual void exitSExpAgregate_proforma(RQLParser::SExpAgregate_proformaContext *ctx) = 0;

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

