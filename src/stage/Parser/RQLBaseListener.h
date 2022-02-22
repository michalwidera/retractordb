
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

  virtual void enterRationalAsFraction(RQLParser::RationalAsFractionContext * /*ctx*/) override { }
  virtual void exitRationalAsFraction(RQLParser::RationalAsFractionContext * /*ctx*/) override { }

  virtual void enterRationalAsFloat(RQLParser::RationalAsFloatContext * /*ctx*/) override { }
  virtual void exitRationalAsFloat(RQLParser::RationalAsFloatContext * /*ctx*/) override { }

  virtual void enterRationalAsDecimal(RQLParser::RationalAsDecimalContext * /*ctx*/) override { }
  virtual void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext * /*ctx*/) override { }

  virtual void enterFraction(RQLParser::FractionContext * /*ctx*/) override { }
  virtual void exitFraction(RQLParser::FractionContext * /*ctx*/) override { }

  virtual void enterDeclare(RQLParser::DeclareContext * /*ctx*/) override { }
  virtual void exitDeclare(RQLParser::DeclareContext * /*ctx*/) override { }

  virtual void enterDeclarationList(RQLParser::DeclarationListContext * /*ctx*/) override { }
  virtual void exitDeclarationList(RQLParser::DeclarationListContext * /*ctx*/) override { }

  virtual void enterSingleDeclaration(RQLParser::SingleDeclarationContext * /*ctx*/) override { }
  virtual void exitSingleDeclaration(RQLParser::SingleDeclarationContext * /*ctx*/) override { }

  virtual void enterField_type(RQLParser::Field_typeContext * /*ctx*/) override { }
  virtual void exitField_type(RQLParser::Field_typeContext * /*ctx*/) override { }

  virtual void enterSelectListFullscan(RQLParser::SelectListFullscanContext * /*ctx*/) override { }
  virtual void exitSelectListFullscan(RQLParser::SelectListFullscanContext * /*ctx*/) override { }

  virtual void enterSelectList(RQLParser::SelectListContext * /*ctx*/) override { }
  virtual void exitSelectList(RQLParser::SelectListContext * /*ctx*/) override { }

  virtual void enterFieldID(RQLParser::FieldIDContext * /*ctx*/) override { }
  virtual void exitFieldID(RQLParser::FieldIDContext * /*ctx*/) override { }

  virtual void enterFieldIDUnderline(RQLParser::FieldIDUnderlineContext * /*ctx*/) override { }
  virtual void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext * /*ctx*/) override { }

  virtual void enterFieldIDColumnname(RQLParser::FieldIDColumnnameContext * /*ctx*/) override { }
  virtual void exitFieldIDColumnname(RQLParser::FieldIDColumnnameContext * /*ctx*/) override { }

  virtual void enterFieldIDTable(RQLParser::FieldIDTableContext * /*ctx*/) override { }
  virtual void exitFieldIDTable(RQLParser::FieldIDTableContext * /*ctx*/) override { }

  virtual void enterUnary_op_expression(RQLParser::Unary_op_expressionContext * /*ctx*/) override { }
  virtual void exitUnary_op_expression(RQLParser::Unary_op_expressionContext * /*ctx*/) override { }

  virtual void enterAsterisk(RQLParser::AsteriskContext * /*ctx*/) override { }
  virtual void exitAsterisk(RQLParser::AsteriskContext * /*ctx*/) override { }

  virtual void enterExpPlus(RQLParser::ExpPlusContext * /*ctx*/) override { }
  virtual void exitExpPlus(RQLParser::ExpPlusContext * /*ctx*/) override { }

  virtual void enterExpMinus(RQLParser::ExpMinusContext * /*ctx*/) override { }
  virtual void exitExpMinus(RQLParser::ExpMinusContext * /*ctx*/) override { }

  virtual void enterExpTerm(RQLParser::ExpTermContext * /*ctx*/) override { }
  virtual void exitExpTerm(RQLParser::ExpTermContext * /*ctx*/) override { }

  virtual void enterExpIn(RQLParser::ExpInContext * /*ctx*/) override { }
  virtual void exitExpIn(RQLParser::ExpInContext * /*ctx*/) override { }

  virtual void enterExpDiv(RQLParser::ExpDivContext * /*ctx*/) override { }
  virtual void exitExpDiv(RQLParser::ExpDivContext * /*ctx*/) override { }

  virtual void enterExpFactor(RQLParser::ExpFactorContext * /*ctx*/) override { }
  virtual void exitExpFactor(RQLParser::ExpFactorContext * /*ctx*/) override { }

  virtual void enterExpMult(RQLParser::ExpMultContext * /*ctx*/) override { }
  virtual void exitExpMult(RQLParser::ExpMultContext * /*ctx*/) override { }

  virtual void enterExpFloat(RQLParser::ExpFloatContext * /*ctx*/) override { }
  virtual void exitExpFloat(RQLParser::ExpFloatContext * /*ctx*/) override { }

  virtual void enterExpDec(RQLParser::ExpDecContext * /*ctx*/) override { }
  virtual void exitExpDec(RQLParser::ExpDecContext * /*ctx*/) override { }

  virtual void enterExpUnary(RQLParser::ExpUnaryContext * /*ctx*/) override { }
  virtual void exitExpUnary(RQLParser::ExpUnaryContext * /*ctx*/) override { }

  virtual void enterExpFnCall(RQLParser::ExpFnCallContext * /*ctx*/) override { }
  virtual void exitExpFnCall(RQLParser::ExpFnCallContext * /*ctx*/) override { }

  virtual void enterExpField(RQLParser::ExpFieldContext * /*ctx*/) override { }
  virtual void exitExpField(RQLParser::ExpFieldContext * /*ctx*/) override { }

  virtual void enterExpAgg(RQLParser::ExpAggContext * /*ctx*/) override { }
  virtual void exitExpAgg(RQLParser::ExpAggContext * /*ctx*/) override { }

  virtual void enterSExpTimeMove(RQLParser::SExpTimeMoveContext * /*ctx*/) override { }
  virtual void exitSExpTimeMove(RQLParser::SExpTimeMoveContext * /*ctx*/) override { }

  virtual void enterSExpPlus(RQLParser::SExpPlusContext * /*ctx*/) override { }
  virtual void exitSExpPlus(RQLParser::SExpPlusContext * /*ctx*/) override { }

  virtual void enterSExpMinus(RQLParser::SExpMinusContext * /*ctx*/) override { }
  virtual void exitSExpMinus(RQLParser::SExpMinusContext * /*ctx*/) override { }

  virtual void enterSExpTerm(RQLParser::SExpTermContext * /*ctx*/) override { }
  virtual void exitSExpTerm(RQLParser::SExpTermContext * /*ctx*/) override { }

  virtual void enterSExpHash(RQLParser::SExpHashContext * /*ctx*/) override { }
  virtual void exitSExpHash(RQLParser::SExpHashContext * /*ctx*/) override { }

  virtual void enterSExpAnd(RQLParser::SExpAndContext * /*ctx*/) override { }
  virtual void exitSExpAnd(RQLParser::SExpAndContext * /*ctx*/) override { }

  virtual void enterSExpMod(RQLParser::SExpModContext * /*ctx*/) override { }
  virtual void exitSExpMod(RQLParser::SExpModContext * /*ctx*/) override { }

  virtual void enterSExpAgse(RQLParser::SExpAgseContext * /*ctx*/) override { }
  virtual void exitSExpAgse(RQLParser::SExpAgseContext * /*ctx*/) override { }

  virtual void enterSExpAgregate(RQLParser::SExpAgregateContext * /*ctx*/) override { }
  virtual void exitSExpAgregate(RQLParser::SExpAgregateContext * /*ctx*/) override { }

  virtual void enterSExpFactor(RQLParser::SExpFactorContext * /*ctx*/) override { }
  virtual void exitSExpFactor(RQLParser::SExpFactorContext * /*ctx*/) override { }

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

