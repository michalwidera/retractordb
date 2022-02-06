
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "RQLListener.h"


/**
 * This class provides an empty implementation of RQLListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  RQLBaseListener : public RQLListener {
public:

  virtual void enterProg(RQLParser::ProgContext * /*ctx*/) override { }
  virtual void exitProg(RQLParser::ProgContext * /*ctx*/) override { }

  virtual void enterSelect(RQLParser::SelectContext * /*ctx*/) override { }
  virtual void exitSelect(RQLParser::SelectContext * /*ctx*/) override { }

  virtual void enterRational(RQLParser::RationalContext * /*ctx*/) override { }
  virtual void exitRational(RQLParser::RationalContext * /*ctx*/) override { }

  virtual void enterDeclare(RQLParser::DeclareContext * /*ctx*/) override { }
  virtual void exitDeclare(RQLParser::DeclareContext * /*ctx*/) override { }

  virtual void enterDeclarationList(RQLParser::DeclarationListContext * /*ctx*/) override { }
  virtual void exitDeclarationList(RQLParser::DeclarationListContext * /*ctx*/) override { }

  virtual void enterSingleDeclaration(RQLParser::SingleDeclarationContext * /*ctx*/) override { }
  virtual void exitSingleDeclaration(RQLParser::SingleDeclarationContext * /*ctx*/) override { }

  virtual void enterField_type(RQLParser::Field_typeContext * /*ctx*/) override { }
  virtual void exitField_type(RQLParser::Field_typeContext * /*ctx*/) override { }

  virtual void enterSelect_list(RQLParser::Select_listContext * /*ctx*/) override { }
  virtual void exitSelect_list(RQLParser::Select_listContext * /*ctx*/) override { }

  virtual void enterSelect_elem(RQLParser::Select_elemContext * /*ctx*/) override { }
  virtual void exitSelect_elem(RQLParser::Select_elemContext * /*ctx*/) override { }

  virtual void enterField_id(RQLParser::Field_idContext * /*ctx*/) override { }
  virtual void exitField_id(RQLParser::Field_idContext * /*ctx*/) override { }

  virtual void enterUnary_op_expression(RQLParser::Unary_op_expressionContext * /*ctx*/) override { }
  virtual void exitUnary_op_expression(RQLParser::Unary_op_expressionContext * /*ctx*/) override { }

  virtual void enterAsterisk(RQLParser::AsteriskContext * /*ctx*/) override { }
  virtual void exitAsterisk(RQLParser::AsteriskContext * /*ctx*/) override { }

  virtual void enterExpression(RQLParser::ExpressionContext * /*ctx*/) override { }
  virtual void exitExpression(RQLParser::ExpressionContext * /*ctx*/) override { }

  virtual void enterTerm(RQLParser::TermContext * /*ctx*/) override { }
  virtual void exitTerm(RQLParser::TermContext * /*ctx*/) override { }

  virtual void enterFactor(RQLParser::FactorContext * /*ctx*/) override { }
  virtual void exitFactor(RQLParser::FactorContext * /*ctx*/) override { }

  virtual void enterStream_expression(RQLParser::Stream_expressionContext * /*ctx*/) override { }
  virtual void exitStream_expression(RQLParser::Stream_expressionContext * /*ctx*/) override { }

  virtual void enterStream_term(RQLParser::Stream_termContext * /*ctx*/) override { }
  virtual void exitStream_term(RQLParser::Stream_termContext * /*ctx*/) override { }

  virtual void enterStream_factor(RQLParser::Stream_factorContext * /*ctx*/) override { }
  virtual void exitStream_factor(RQLParser::Stream_factorContext * /*ctx*/) override { }

  virtual void enterAgregator(RQLParser::AgregatorContext * /*ctx*/) override { }
  virtual void exitAgregator(RQLParser::AgregatorContext * /*ctx*/) override { }

  virtual void enterFunction_call(RQLParser::Function_callContext * /*ctx*/) override { }
  virtual void exitFunction_call(RQLParser::Function_callContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

