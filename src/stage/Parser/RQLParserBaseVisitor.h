
// Generated from RQLParser.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "RQLParserVisitor.h"


/**
 * This class provides an empty implementation of RQLParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  RQLParserBaseVisitor : public RQLParserVisitor {
public:

  virtual antlrcpp::Any visitClauses(RQLParser::ClausesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSelect_statement(RQLParser::Select_statementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDeclare_statement(RQLParser::Declare_statementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFull_column_name_list(RQLParser::Full_column_name_listContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSelect_list(RQLParser::Select_listContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStream_expression(RQLParser::Stream_expressionContext *ctx) override {
    return visitChildren(ctx);
  }


};

