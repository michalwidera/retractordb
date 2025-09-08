
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    BYTE_T = 21, STRING_T = 22, UNSIGNED_T = 23, INTEGER_T = 24, FLOAT_T = 25, 
    DOUBLE_T = 26, SELECT = 27, STREAM = 28, FROM = 29, DECLARE = 30, RETENTION = 31, 
    FILE = 32, STORAGE = 33, SUBSTRAT = 34, RULE = 35, ON = 36, WHEN = 37, 
    DUMP = 38, SYSTEM = 39, DO = 40, TO = 41, AND_C = 42, OR_C = 43, NOT_C = 44, 
    MIN = 45, MAX = 46, AVG = 47, SUMC = 48, STRING_SUBSTRAT = 49, ID = 50, 
    STRING = 51, FLOAT = 52, DECIMAL = 53, REAL = 54, IS_EQ = 55, IS_NQ = 56, 
    IS_GR = 57, IS_LS = 58, IS_GE = 59, IS_LE = 60, EXCLAMATION = 61, DOUBLE_BAR = 62, 
    DOT = 63, UNDERLINE = 64, AT = 65, SHARP = 66, AND = 67, MOD = 68, DOLLAR = 69, 
    COMMA = 70, SEMI = 71, COLON = 72, DOUBLE_COLON = 73, STAR = 74, DIVIDE = 75, 
    PLUS = 76, MINUS = 77, BIT_NOT = 78, BIT_OR = 79, BIT_XOR = 80, SPACE = 81, 
    COMMENT = 82, LINE_COMMENT1 = 83, LINE_COMMENT2 = 84
  };

  enum {
    RuleProg = 0, RuleStorage_statement = 1, RuleSubstrat_statement = 2, 
    RuleSelect_statement = 3, RuleDeclare_statement = 4, RuleRule_statement = 5, 
    RuleDumppart = 6, RuleSystempart = 7, RuleRational_se = 8, RuleRetention_from = 9, 
    RuleFraction_rule = 10, RuleField_declaration = 11, RuleField_type = 12, 
    RuleSelect_list = 13, RuleField_id = 14, RuleUnary_op_expression = 15, 
    RuleAsterisk = 16, RuleExpression = 17, RuleLogic = 18, RuleExpression_logic = 19, 
    RuleTerm_logic = 20, RuleExpression_factor = 21, RuleTerm = 22, RuleStream_expression = 23, 
    RuleStream_term = 24, RuleStream_factor = 25, RuleAgregator = 26, RuleFunction_call = 27
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
  class Substrat_statementContext;
  class Select_statementContext;
  class Declare_statementContext;
  class Rule_statementContext;
  class DumppartContext;
  class SystempartContext;
  class Rational_seContext;
  class Retention_fromContext;
  class Fraction_ruleContext;
  class Field_declarationContext;
  class Field_typeContext;
  class Select_listContext;
  class Field_idContext;
  class Unary_op_expressionContext;
  class AsteriskContext;
  class ExpressionContext;
  class LogicContext;
  class Expression_logicContext;
  class Term_logicContext;
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
    std::vector<Substrat_statementContext *> substrat_statement();
    Substrat_statementContext* substrat_statement(size_t i);
    std::vector<Rule_statementContext *> rule_statement();
    Rule_statementContext* rule_statement(size_t i);

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

  class  Substrat_statementContext : public antlr4::ParserRuleContext {
  public:
    Substrat_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Substrat_statementContext() = default;
    void copyFrom(Substrat_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SubstratContext : public Substrat_statementContext {
  public:
    SubstratContext(Substrat_statementContext *ctx);

    antlr4::Token *substrat_type = nullptr;
    antlr4::tree::TerminalNode *SUBSTRAT();
    antlr4::tree::TerminalNode *STRING_SUBSTRAT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Substrat_statementContext* substrat_statement();

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

    antlr4::Token *stream_name = nullptr;
    antlr4::Token *name = nullptr;
    antlr4::tree::TerminalNode *SELECT();
    Select_listContext *select_list();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *FROM();
    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *FILE();
    Retention_fromContext *retention_from();
    antlr4::tree::TerminalNode *STRING();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Select_statementContext* select_statement();

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
    std::vector<Field_declarationContext *> field_declaration();
    Field_declarationContext* field_declaration(size_t i);
    antlr4::tree::TerminalNode *STREAM();
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);
    Rational_seContext *rational_se();
    antlr4::tree::TerminalNode *FILE();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *STRING();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Declare_statementContext* declare_statement();

  class  Rule_statementContext : public antlr4::ParserRuleContext {
  public:
    Rule_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Rule_statementContext() = default;
    void copyFrom(Rule_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  RulezContext : public Rule_statementContext {
  public:
    RulezContext(Rule_statementContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *stream_name = nullptr;
    antlr4::tree::TerminalNode *RULE();
    antlr4::tree::TerminalNode *ON();
    antlr4::tree::TerminalNode *WHEN();
    LogicContext *logic();
    antlr4::tree::TerminalNode *DO();
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    DumppartContext *dumppart();
    SystempartContext *systempart();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Rule_statementContext* rule_statement();

  class  DumppartContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *step_back = nullptr;
    antlr4::Token *step_forward = nullptr;
    antlr4::Token *rule_retnetion = nullptr;
    DumppartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DUMP();
    antlr4::tree::TerminalNode *TO();
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MINUS();
    antlr4::tree::TerminalNode* MINUS(size_t i);
    antlr4::tree::TerminalNode *RETENTION();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  DumppartContext* dumppart();

  class  SystempartContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *syscmd = nullptr;
    SystempartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SYSTEM();
    antlr4::tree::TerminalNode *STRING();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  SystempartContext* systempart();

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

    Fraction_ruleContext *fraction_rule();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Rational_seContext* rational_se();

  class  Retention_fromContext : public antlr4::ParserRuleContext {
  public:
    Retention_fromContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Retention_fromContext() = default;
    void copyFrom(Retention_fromContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  RetentionContext : public Retention_fromContext {
  public:
    RetentionContext(Retention_fromContext *ctx);

    antlr4::Token *capacity = nullptr;
    antlr4::Token *segments = nullptr;
    antlr4::tree::TerminalNode *RETENTION();
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Retention_fromContext* retention_from();

  class  Fraction_ruleContext : public antlr4::ParserRuleContext {
  public:
    Fraction_ruleContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Fraction_ruleContext() = default;
    void copyFrom(Fraction_ruleContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  FractionContext : public Fraction_ruleContext {
  public:
    FractionContext(Fraction_ruleContext *ctx);

    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    antlr4::tree::TerminalNode *DIVIDE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Fraction_ruleContext* fraction_rule();

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

    antlr4::Token *type_size = nullptr;
    antlr4::tree::TerminalNode *ID();
    Field_typeContext *field_type();
    antlr4::tree::TerminalNode *DECIMAL();
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

  class  TypeStringContext : public Field_typeContext {
  public:
    TypeStringContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *STRING_T();
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

  class  TypeDoubleContext : public Field_typeContext {
  public:
    TypeDoubleContext(Field_typeContext *ctx);

    antlr4::tree::TerminalNode *DOUBLE_T();
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

  class  LogicContext : public antlr4::ParserRuleContext {
  public:
    LogicContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    LogicContext() = default;
    void copyFrom(LogicContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  LogicExpressionContext : public LogicContext {
  public:
    LogicExpressionContext(LogicContext *ctx);

    Expression_logicContext *expression_logic();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  LogicContext* logic();

  class  Expression_logicContext : public antlr4::ParserRuleContext {
  public:
    Expression_logicContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Expression_logicContext() = default;
    void copyFrom(Expression_logicContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpOrContext : public Expression_logicContext {
  public:
    ExpOrContext(Expression_logicContext *ctx);

    std::vector<Expression_logicContext *> expression_logic();
    Expression_logicContext* expression_logic(size_t i);
    antlr4::tree::TerminalNode *OR_C();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpAndContext : public Expression_logicContext {
  public:
    ExpAndContext(Expression_logicContext *ctx);

    std::vector<Expression_logicContext *> expression_logic();
    Expression_logicContext* expression_logic(size_t i);
    antlr4::tree::TerminalNode *AND_C();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpTermLogicContext : public Expression_logicContext {
  public:
    ExpTermLogicContext(Expression_logicContext *ctx);

    Term_logicContext *term_logic();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Expression_logicContext* expression_logic();
  Expression_logicContext* expression_logic(int precedence);
  class  Term_logicContext : public antlr4::ParserRuleContext {
  public:
    Term_logicContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Term_logicContext() = default;
    void copyFrom(Term_logicContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpLsContext : public Term_logicContext {
  public:
    ExpLsContext(Term_logicContext *ctx);

    std::vector<Term_logicContext *> term_logic();
    Term_logicContext* term_logic(size_t i);
    antlr4::tree::TerminalNode *IS_LS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpLeContext : public Term_logicContext {
  public:
    ExpLeContext(Term_logicContext *ctx);

    std::vector<Term_logicContext *> term_logic();
    Term_logicContext* term_logic(size_t i);
    antlr4::tree::TerminalNode *IS_LE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpNqContext : public Term_logicContext {
  public:
    ExpNqContext(Term_logicContext *ctx);

    std::vector<Term_logicContext *> term_logic();
    Term_logicContext* term_logic(size_t i);
    antlr4::tree::TerminalNode *IS_NQ();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpGrContext : public Term_logicContext {
  public:
    ExpGrContext(Term_logicContext *ctx);

    std::vector<Term_logicContext *> term_logic();
    Term_logicContext* term_logic(size_t i);
    antlr4::tree::TerminalNode *IS_GR();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpFactorContext : public Term_logicContext {
  public:
    ExpFactorContext(Term_logicContext *ctx);

    Expression_factorContext *expression_factor();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpEqContext : public Term_logicContext {
  public:
    ExpEqContext(Term_logicContext *ctx);

    std::vector<Term_logicContext *> term_logic();
    Term_logicContext* term_logic(size_t i);
    antlr4::tree::TerminalNode *IS_EQ();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpGeContext : public Term_logicContext {
  public:
    ExpGeContext(Term_logicContext *ctx);

    std::vector<Term_logicContext *> term_logic();
    Term_logicContext* term_logic(size_t i);
    antlr4::tree::TerminalNode *IS_GE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Term_logicContext* term_logic();
  Term_logicContext* term_logic(int precedence);
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

    Fraction_ruleContext *fraction_rule();
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

  class  ExpNotContext : public TermContext {
  public:
    ExpNotContext(TermContext *ctx);

    antlr4::tree::TerminalNode *NOT_C();
    TermContext *term();
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
    antlr4::tree::TerminalNode *IS_GR();
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

    antlr4::Token *step = nullptr;
    antlr4::Token *window = nullptr;
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

  bool expression_logicSempred(Expression_logicContext *_localctx, size_t predicateIndex);
  bool term_logicSempred(Term_logicContext *_localctx, size_t predicateIndex);
  bool expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex);
  bool termSempred(TermContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

