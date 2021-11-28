
    // https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "RQLVisitor.h"


/**
 * This class provides an empty implementation of RQLVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  RQLBaseVisitor : public RQLVisitor {
public:

  virtual antlrcpp::Any visitProg(RQLParser::ProgContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSelect_statement(RQLParser::Select_statementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDeclare_statement(RQLParser::Declare_statementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitColumn_name_list(RQLParser::Column_name_listContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitColumn_type(RQLParser::Column_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSelect_list(RQLParser::Select_listContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSelect_list_elem(RQLParser::Select_list_elemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitField_id(RQLParser::Field_idContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAsterisk(RQLParser::AsteriskContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpression(RQLParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTerm(RQLParser::TermContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFactor(RQLParser::FactorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStream_expression(RQLParser::Stream_expressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStream_term(RQLParser::Stream_termContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAgregator(RQLParser::AgregatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFunction_call(RQLParser::Function_callContext *ctx) override {
    return visitChildren(ctx);
  }


};

