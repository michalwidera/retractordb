
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    STRING_T = 21, BYTEARRAY_T = 22, INTARRAY_T = 23, BYTE_T = 24, UNSIGNED_T = 25, 
    INTEGER_T = 26, FLOAT_T = 27, SELECT = 28, STREAM = 29, FROM = 30, DECLARE = 31, 
    FILE = 32, MIN = 33, MAX = 34, AVG = 35, SUMC = 36, ID = 37, STRING = 38, 
    FLOAT = 39, DECIMAL = 40, REAL = 41, EQUAL = 42, GREATER = 43, LESS = 44, 
    EXCLAMATION = 45, DOUBLE_BAR = 46, DOT = 47, UNDERLINE = 48, AT = 49, 
    SHARP = 50, AND = 51, MOD = 52, DOLLAR = 53, COMMA = 54, SEMI = 55, 
    COLON = 56, DOUBLE_COLON = 57, STAR = 58, DIVIDE = 59, PLUS = 60, MINUS = 61, 
    BIT_NOT = 62, BIT_OR = 63, BIT_XOR = 64, SPACE = 65, COMMENT = 66, LINE_COMMENT = 67
  };

  enum {
    RuleProg = 0, RuleSelect_statement = 1, RuleRational = 2, RuleFraction = 3, 
    RuleDeclare_statement = 4, RuleDeclare_list = 5, RuleField_declaration = 6, 
    RuleField_type = 7, RuleSelect_list = 8, RuleField_id = 9, RuleUnary_op_expression = 10, 
    RuleAsterisk = 11, RuleExpression = 12, RuleExpression_factor = 13, 
    RuleTerm = 14, RuleFactor = 15, RuleStream_expression = 16, RuleStream_term = 17, 
    RuleStream_factor = 18, RuleAgregator = 19, RuleFunction_call = 20
  };

  explicit RQLParser(antlr4::TokenStream *input);
  ~RQLParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class ProgContext;
  class Select_statementContext;
  class RationalContext;
  class FractionContext;
  class Declare_statementContext;
  class Declare_listContext;
  class Field_declarationContext;
  class Field_typeContext;
  class Select_listContext;
  class Field_idContext;
  class Unary_op_expressionContext;
  class AsteriskContext;
  class ExpressionContext;
  class Expression_factorContext;
  class TermContext;
  class FactorContext;
  class Stream_expressionContext;
  class Stream_termContext;
  class Stream_factorContext;
  class AgregatorContext;
  class Function_callContext; 

  class  ProgContext : public antlr4::ParserRuleContext {
  public:
    ProgContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<Select_statementContext *> select_statement();
    Select_statementContext* select_statement(size_t i);
    std::vector<Declare_statementContext *> declare_statement();
    Declare_statementContext* declare_statement(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  ProgContext* prog();

  class  Select_statementContext : public antlr4::ParserRuleContext {
  public:
    Select_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Select_statementContext() = default;
    void copyFrom(Select_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SelectContext : public Select_statementContext {
  public:
    SelectContext(Select_statementContext *ctx);

    antlr4::tree::TerminalNode *SELECT();
    Select_listContext *select_list();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *FROM();
    Stream_expressionContext *stream_expression();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Select_statementContext* select_statement();

  class  RationalContext : public antlr4::ParserRuleContext {
  public:
    RationalContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    RationalContext() = default;
    void copyFrom(RationalContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  RationalAsDecimalContext : public RationalContext {
  public:
    RationalAsDecimalContext(RationalContext *ctx);

    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  RationalAsFloatContext : public RationalContext {
  public:
    RationalAsFloatContext(RationalContext *ctx);

    antlr4::tree::TerminalNode *FLOAT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  RationalAsFraction_proformaContext : public RationalContext {
  public:
    RationalAsFraction_proformaContext(RationalContext *ctx);

    FractionContext *fraction();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  RationalContext* rational();

  class  FractionContext : public antlr4::ParserRuleContext {
  public:
    FractionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    antlr4::tree::TerminalNode *DIVIDE();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  FractionContext* fraction();

  class  Declare_statementContext : public antlr4::ParserRuleContext {
  public:
    Declare_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Declare_statementContext() = default;
    void copyFrom(Declare_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  DeclareContext : public Declare_statementContext {
  public:
    DeclareContext(Declare_statementContext *ctx);

    antlr4::Token *stream_name = nullptr;
    antlr4::Token *file_name = nullptr;
    antlr4::tree::TerminalNode *DECLARE();
    Declare_listContext *declare_list();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *COMMA();
    RationalContext *rational();
    antlr4::tree::TerminalNode *FILE();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *STRING();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Declare_statementContext* declare_statement();

  class  Declare_listContext : public antlr4::ParserRuleContext {
  public:
    Declare_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Declare_listContext() = default;
    void copyFrom(Declare_listContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  DeclarationListContext : public Declare_listContext {
  public:
    DeclarationListContext(Declare_listContext *ctx);

    std::vector<Field_declarationContext *> field_declaration();
    Field_declarationContext* field_declaration(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Declare_listContext* declare_list();

  class  Field_declarationContext : public antlr4::ParserRuleContext {
  public:
    Field_declarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Field_declarationContext() = default;
    void copyFrom(Field_declarationContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SingleDeclarationContext : public Field_declarationContext {
  public:
    SingleDeclarationContext(Field_declarationContext *ctx);

    antlr4::tree::TerminalNode *ID();
    Field_typeContext *field_type();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Field_declarationContext* field_declaration();

  class  Field_typeContext : public antlr4::ParserRuleContext {
  public:
    Field_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Field_typeContext() = default;
    void copyFrom(Field_typeContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  TypeArrayContext : public Field_typeContext {
  public:
    TypeArrayContext(Field_typeContext *ctx);

    antlr4::Token *type_size = nullptr;
    antlr4::tree::TerminalNode *STRING_T();
    antlr4::tree::TerminalNode *INTARRAY_T();
    antlr4::tree::TerminalNode *BYTEARRAY_T();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  TypeIntContext : public Field_typeContext {
  public:
    TypeIntContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *INTEGER_T();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  TypeFloatContext : public Field_typeContext {
  public:
    TypeFloatContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *FLOAT_T();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  TypeUnsigedContext : public Field_typeContext {
  public:
    TypeUnsigedContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *UNSIGNED_T();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  TypeByteContext : public Field_typeContext {
  public:
    TypeByteContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *BYTE_T();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Field_typeContext* field_type();

  class  Select_listContext : public antlr4::ParserRuleContext {
  public:
    Select_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Select_listContext() = default;
    void copyFrom(Select_listContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SelectListContext : public Select_listContext {
  public:
    SelectListContext(Select_listContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SelectListFullscanContext : public Select_listContext {
  public:
    SelectListFullscanContext(Select_listContext *ctx);

    AsteriskContext *asterisk();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Select_listContext* select_list();

  class  Field_idContext : public antlr4::ParserRuleContext {
  public:
    Field_idContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Field_idContext() = default;
    void copyFrom(Field_idContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  FieldIDUnderlineContext : public Field_idContext {
  public:
    FieldIDUnderlineContext(Field_idContext *ctx);

    antlr4::Token *tablename = nullptr;
    antlr4::tree::TerminalNode *UNDERLINE();
    antlr4::tree::TerminalNode *ID();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  FieldIDTableContext : public Field_idContext {
  public:
    FieldIDTableContext(Field_idContext *ctx);

    antlr4::Token *tablename = nullptr;
    antlr4::Token *column_index = nullptr;
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  FieldIDContext : public Field_idContext {
  public:
    FieldIDContext(Field_idContext *ctx);

    antlr4::Token *column_name = nullptr;
    antlr4::tree::TerminalNode *ID();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  FieldIDColumnnameContext : public Field_idContext {
  public:
    FieldIDColumnnameContext(Field_idContext *ctx);

    antlr4::Token *tablename = nullptr;
    antlr4::Token *column_name = nullptr;
    antlr4::tree::TerminalNode *DOT();
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Field_idContext* field_id();

  class  Unary_op_expressionContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *op = nullptr;
    Unary_op_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BIT_NOT();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Unary_op_expressionContext* unary_op_expression();

  class  AsteriskContext : public antlr4::ParserRuleContext {
  public:
    AsteriskContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *STAR();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DOT();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  AsteriskContext* asterisk();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Expression_factorContext *expression_factor();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  ExpressionContext* expression();

  class  Expression_factorContext : public antlr4::ParserRuleContext {
  public:
    Expression_factorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Expression_factorContext() = default;
    void copyFrom(Expression_factorContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpPlusContext : public Expression_factorContext {
  public:
    ExpPlusContext(Expression_factorContext *ctx);

    std::vector<Expression_factorContext *> expression_factor();
    Expression_factorContext* expression_factor(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpMinusContext : public Expression_factorContext {
  public:
    ExpMinusContext(Expression_factorContext *ctx);

    std::vector<Expression_factorContext *> expression_factor();
    Expression_factorContext* expression_factor(size_t i);
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpTermContext : public Expression_factorContext {
  public:
    ExpTermContext(Expression_factorContext *ctx);

    TermContext *term();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Expression_factorContext* expression_factor();
  Expression_factorContext* expression_factor(int precedence);
  class  TermContext : public antlr4::ParserRuleContext {
  public:
    TermContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    TermContext() = default;
    void copyFrom(TermContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpInContext : public TermContext {
  public:
    ExpInContext(TermContext *ctx);

    Expression_factorContext *expression_factor();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpDivContext : public TermContext {
  public:
    ExpDivContext(TermContext *ctx);

    std::vector<TermContext *> term();
    TermContext* term(size_t i);
    antlr4::tree::TerminalNode *DIVIDE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpFactorContext : public TermContext {
  public:
    ExpFactorContext(TermContext *ctx);

    FactorContext *factor();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpMultContext : public TermContext {
  public:
    ExpMultContext(TermContext *ctx);

    std::vector<TermContext *> term();
    TermContext* term(size_t i);
    antlr4::tree::TerminalNode *STAR();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  TermContext* term();
  TermContext* term(int precedence);
  class  FactorContext : public antlr4::ParserRuleContext {
  public:
    FactorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    FactorContext() = default;
    void copyFrom(FactorContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpFloatContext : public FactorContext {
  public:
    ExpFloatContext(FactorContext *ctx);

    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpDecContext : public FactorContext {
  public:
    ExpDecContext(FactorContext *ctx);

    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpAggContext : public FactorContext {
  public:
    ExpAggContext(FactorContext *ctx);

    AgregatorContext *agregator();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpFnCallContext : public FactorContext {
  public:
    ExpFnCallContext(FactorContext *ctx);

    Function_callContext *function_call();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpFieldContext : public FactorContext {
  public:
    ExpFieldContext(FactorContext *ctx);

    Field_idContext *field_id();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpUnaryContext : public FactorContext {
  public:
    ExpUnaryContext(FactorContext *ctx);

    Unary_op_expressionContext *unary_op_expression();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  FactorContext* factor();

  class  Stream_expressionContext : public antlr4::ParserRuleContext {
  public:
    Stream_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Stream_expressionContext() = default;
    void copyFrom(Stream_expressionContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SExpPlusContext : public Stream_expressionContext {
  public:
    SExpPlusContext(Stream_expressionContext *ctx);

    std::vector<Stream_termContext *> stream_term();
    Stream_termContext* stream_term(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpTermContext : public Stream_expressionContext {
  public:
    SExpTermContext(Stream_expressionContext *ctx);

    Stream_termContext *stream_term();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpTimeMoveContext : public Stream_expressionContext {
  public:
    SExpTimeMoveContext(Stream_expressionContext *ctx);

    Stream_termContext *stream_term();
    antlr4::tree::TerminalNode *GREATER();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpMinusContext : public Stream_expressionContext {
  public:
    SExpMinusContext(Stream_expressionContext *ctx);

    Stream_termContext *stream_term();
    antlr4::tree::TerminalNode *MINUS();
    RationalContext *rational();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Stream_expressionContext* stream_expression();

  class  Stream_termContext : public antlr4::ParserRuleContext {
  public:
    Stream_termContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Stream_termContext() = default;
    void copyFrom(Stream_termContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SExpFactorContext : public Stream_termContext {
  public:
    SExpFactorContext(Stream_termContext *ctx);

    Stream_factorContext *stream_factor();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpHashContext : public Stream_termContext {
  public:
    SExpHashContext(Stream_termContext *ctx);

    std::vector<Stream_factorContext *> stream_factor();
    Stream_factorContext* stream_factor(size_t i);
    antlr4::tree::TerminalNode *SHARP();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpModContext : public Stream_termContext {
  public:
    SExpModContext(Stream_termContext *ctx);

    Stream_factorContext *stream_factor();
    antlr4::tree::TerminalNode *MOD();
    RationalContext *rational();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpAgregate_proformaContext : public Stream_termContext {
  public:
    SExpAgregate_proformaContext(Stream_termContext *ctx);

    Stream_factorContext *stream_factor();
    antlr4::tree::TerminalNode *DOT();
    AgregatorContext *agregator();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpAgseContext : public Stream_termContext {
  public:
    SExpAgseContext(Stream_termContext *ctx);

    antlr4::Token *window = nullptr;
    antlr4::Token *step = nullptr;
    Stream_factorContext *stream_factor();
    antlr4::tree::TerminalNode *AT();
    antlr4::tree::TerminalNode *COMMA();
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpAndContext : public Stream_termContext {
  public:
    SExpAndContext(Stream_termContext *ctx);

    Stream_factorContext *stream_factor();
    antlr4::tree::TerminalNode *AND();
    RationalContext *rational();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Stream_termContext* stream_term();

  class  Stream_factorContext : public antlr4::ParserRuleContext {
  public:
    Stream_factorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();
    Stream_expressionContext *stream_expression();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Stream_factorContext* stream_factor();

  class  AgregatorContext : public antlr4::ParserRuleContext {
  public:
    AgregatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    AgregatorContext() = default;
    void copyFrom(AgregatorContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  StreamMinContext : public AgregatorContext {
  public:
    StreamMinContext(AgregatorContext *ctx);

    antlr4::tree::TerminalNode *MIN();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  StreamAvgContext : public AgregatorContext {
  public:
    StreamAvgContext(AgregatorContext *ctx);

    antlr4::tree::TerminalNode *AVG();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  StreamMaxContext : public AgregatorContext {
  public:
    StreamMaxContext(AgregatorContext *ctx);

    antlr4::tree::TerminalNode *MAX();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  StreamSumContext : public AgregatorContext {
  public:
    StreamSumContext(AgregatorContext *ctx);

    antlr4::tree::TerminalNode *SUMC();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  AgregatorContext* agregator();

  class  Function_callContext : public antlr4::ParserRuleContext {
  public:
    Function_callContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Expression_factorContext *> expression_factor();
    Expression_factorContext* expression_factor(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Function_callContext* function_call();


  virtual bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;
  bool expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex);
  bool termSempred(TermContext *_localctx, size_t predicateIndex);

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

