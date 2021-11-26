
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "RQLParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by RQLParser.
 */
class  RQLVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by RQLParser.
   */
    virtual antlrcpp::Any visitClauses(RQLParser::ClausesContext *context) = 0;

    virtual antlrcpp::Any visitSelect_statement(RQLParser::Select_statementContext *context) = 0;

    virtual antlrcpp::Any visitDeclare_statement(RQLParser::Declare_statementContext *context) = 0;

    virtual antlrcpp::Any visitColumn_name_list(RQLParser::Column_name_listContext *context) = 0;

    virtual antlrcpp::Any visitSelect_list(RQLParser::Select_listContext *context) = 0;

    virtual antlrcpp::Any visitStream_expression(RQLParser::Stream_expressionContext *context) = 0;


};

