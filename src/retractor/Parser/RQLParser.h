
// Generated from RQL.g4 by ANTLR 4.11.1

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
    FILE = 32, STORAGE = 33, MIN = 34, MAX = 35, AVG = 36, SUMC = 37, ID = 38, 
    STRING = 39, FLOAT = 40, DECIMAL = 41, REAL = 42, EQUAL = 43, GREATER = 44, 
    LESS = 45, EXCLAMATION = 46, DOUBLE_BAR = 47, DOT = 48, UNDERLINE = 49, 
    AT = 50, SHARP = 51, AND = 52, MOD = 53, DOLLAR = 54, COMMA = 55, SEMI = 56, 
    COLON = 57, DOUBLE_COLON = 58, STAR = 59, DIVIDE = 60, PLUS = 61, MINUS = 62, 
    BIT_NOT = 63, BIT_OR = 64, BIT_XOR = 65, SPACE = 66, COMMENT = 67, LINE_COMMENT = 68
  };

  enum {
    RuleProg = 0, RuleStorage_statement = 1, RuleSelect_statement = 2, RuleRational_se = 3, 
    RuleFraction = 4, RuleDeclare_statement = 5, RuleDeclare_list = 6, RuleField_declaration = 7, 
    RuleField_type = 8, RuleSelect_list = 9, RuleField_id = 10, RuleUnary_op_expression = 11, 
    RuleAsterisk = 12, RuleExpression = 13, RuleExpression_factor = 14, 
    RuleTerm = 15, RuleStream_expression = 16, RuleStream_term = 17, RuleStream_factor = 18, 
    RuleAgregator = 19, RuleFunction_call = 20
  };

  explicit RQLParser(antlr4::TokenStream *input);

  RQLParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~RQLParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class ProgContext;
  class Storage_statementContext;
  class Select_statementContext;
  class Rational_seContext;
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
    std::vector<Storage_statementContext *> storage_statement();
    Storage_statementContext* storage_statement(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  ProgContext* prog();

  class  Storage_statementContext : public antlr4::ParserRuleContext {
  public:
    Storage_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Storage_statementContext() = default;
    void copyFrom(Storage_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  StorageContext : public Storage_statementContext {
  public:
    StorageContext(Storage_statementContext *ctx);

    antlr4::Token *folder_name = nullptr;
    antlr4::tree::TerminalNode *STORAGE();
    antlr4::tree::TerminalNode *STRING();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Storage_statementContext* storage_statement();

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

  class  Rational_seContext : public antlr4::ParserRuleContext {
  public:
    Rational_seContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Rational_seContext() = default;
    void copyFrom(Rational_seContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  RationalAsDecimalContext : public Rational_seContext {
  public:
    RationalAsDecimalContext(Rational_seContext *ctx);

    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  RationalAsFloatContext : public Rational_seContext {
  public:
    RationalAsFloatContext(Rational_seContext *ctx);

    antlr4::tree::TerminalNode *FLOAT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  RationalAsFraction_proformaContext : public Rational_seContext {
  public:
    RationalAsFraction_proformaContext(Rational_seContext *ctx);

    FractionContext *fraction();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Rational_seContext* rational_se();

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
    Rational_seContext *rational_se();
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

  class  TypeUnsignedContext : public Field_typeContext {
  public:
    TypeUnsignedContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *UNSIGNED_T();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
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

  class  FieldIDColumnNameContext : public Field_idContext {
  public:
    FieldIDColumnNameContext(Field_idContext *ctx);

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

  class  ExpRationalContext : public TermContext {
  public:
    ExpRationalContext(TermContext *ctx);

    FractionContext *fraction();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpFloatContext : public TermContext {
  public:
    ExpFloatContext(TermContext *ctx);

    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpDecContext : public TermContext {
  public:
    ExpDecContext(TermContext *ctx);

    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpAggContext : public TermContext {
  public:
    ExpAggContext(TermContext *ctx);

    AgregatorContext *agregator();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpFnCallContext : public TermContext {
  public:
    ExpFnCallContext(TermContext *ctx);

    Function_callContext *function_call();
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

  class  ExpFieldContext : public TermContext {
  public:
    ExpFieldContext(TermContext *ctx);

    Field_idContext *field_id();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpStringContext : public TermContext {
  public:
    ExpStringContext(TermContext *ctx);

    antlr4::tree::TerminalNode *STRING();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpUnaryContext : public TermContext {
  public:
    ExpUnaryContext(TermContext *ctx);

    Unary_op_expressionContext *unary_op_expression();
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
    Rational_seContext *rational_se();
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
    Rational_seContext *rational_se();
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
    Rational_seContext *rational_se();
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


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex);
  bool termSempred(TermContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

