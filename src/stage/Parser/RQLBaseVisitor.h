
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

  virtual antlrcpp::Any visitClauses(RQLParser::ClausesContext *ctx) override {
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

  virtual antlrcpp::Any visitSelect_list(RQLParser::Select_listContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStream_expression(RQLParser::Stream_expressionContext *ctx) override {
    return visitChildren(ctx);
  }


};

