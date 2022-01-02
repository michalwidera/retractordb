
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

  virtual void enterRational(RQLParser::RationalContext *ctx) = 0;
  virtual void exitRational(RQLParser::RationalContext *ctx) = 0;

  virtual void enterDeclare(RQLParser::DeclareContext *ctx) = 0;
  virtual void exitDeclare(RQLParser::DeclareContext *ctx) = 0;

  virtual void enterDeclarationList(RQLParser::DeclarationListContext *ctx) = 0;
  virtual void exitDeclarationList(RQLParser::DeclarationListContext *ctx) = 0;

  virtual void enterDeclare_type(RQLParser::Declare_typeContext *ctx) = 0;
  virtual void exitDeclare_type(RQLParser::Declare_typeContext *ctx) = 0;

  virtual void enterSelect_list(RQLParser::Select_listContext *ctx) = 0;
  virtual void exitSelect_list(RQLParser::Select_listContext *ctx) = 0;

  virtual void enterSelect_elem(RQLParser::Select_elemContext *ctx) = 0;
  virtual void exitSelect_elem(RQLParser::Select_elemContext *ctx) = 0;

  virtual void enterField_id(RQLParser::Field_idContext *ctx) = 0;
  virtual void exitField_id(RQLParser::Field_idContext *ctx) = 0;

  virtual void enterUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) = 0;
  virtual void exitUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) = 0;

  virtual void enterAsterisk(RQLParser::AsteriskContext *ctx) = 0;
  virtual void exitAsterisk(RQLParser::AsteriskContext *ctx) = 0;

  virtual void enterExpression(RQLParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(RQLParser::ExpressionContext *ctx) = 0;

  virtual void enterTerm(RQLParser::TermContext *ctx) = 0;
  virtual void exitTerm(RQLParser::TermContext *ctx) = 0;

  virtual void enterFactor(RQLParser::FactorContext *ctx) = 0;
  virtual void exitFactor(RQLParser::FactorContext *ctx) = 0;

  virtual void enterStream_expression(RQLParser::Stream_expressionContext *ctx) = 0;
  virtual void exitStream_expression(RQLParser::Stream_expressionContext *ctx) = 0;

  virtual void enterStream_term(RQLParser::Stream_termContext *ctx) = 0;
  virtual void exitStream_term(RQLParser::Stream_termContext *ctx) = 0;

  virtual void enterStream_factor(RQLParser::Stream_factorContext *ctx) = 0;
  virtual void exitStream_factor(RQLParser::Stream_factorContext *ctx) = 0;

  virtual void enterAgregator(RQLParser::AgregatorContext *ctx) = 0;
  virtual void exitAgregator(RQLParser::AgregatorContext *ctx) = 0;

  virtual void enterFunction_call(RQLParser::Function_callContext *ctx) = 0;
  virtual void exitFunction_call(RQLParser::Function_callContext *ctx) = 0;


};

