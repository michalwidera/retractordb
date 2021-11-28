
    #include <iostream>


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
    virtual antlrcpp::Any visitProg(RQLParser::ProgContext *context) = 0;

    virtual antlrcpp::Any visitSelect_statement(RQLParser::Select_statementContext *context) = 0;

    virtual antlrcpp::Any visitDeclare_statement(RQLParser::Declare_statementContext *context) = 0;

    virtual antlrcpp::Any visitColumn_name_list(RQLParser::Column_name_listContext *context) = 0;

    virtual antlrcpp::Any visitColumn_type(RQLParser::Column_typeContext *context) = 0;

    virtual antlrcpp::Any visitSelect_list(RQLParser::Select_listContext *context) = 0;

    virtual antlrcpp::Any visitSelect_list_elem(RQLParser::Select_list_elemContext *context) = 0;

    virtual antlrcpp::Any visitField_id(RQLParser::Field_idContext *context) = 0;

    virtual antlrcpp::Any visitUnary_op_expression(RQLParser::Unary_op_expressionContext *context) = 0;

    virtual antlrcpp::Any visitAsterisk(RQLParser::AsteriskContext *context) = 0;

    virtual antlrcpp::Any visitExpression(RQLParser::ExpressionContext *context) = 0;

    virtual antlrcpp::Any visitTerm(RQLParser::TermContext *context) = 0;

    virtual antlrcpp::Any visitFactor(RQLParser::FactorContext *context) = 0;

    virtual antlrcpp::Any visitStream_expression(RQLParser::Stream_expressionContext *context) = 0;

    virtual antlrcpp::Any visitStream_term(RQLParser::Stream_termContext *context) = 0;

    virtual antlrcpp::Any visitAgregator(RQLParser::AgregatorContext *context) = 0;

    virtual antlrcpp::Any visitFunction_call(RQLParser::Function_callContext *context) = 0;


};

