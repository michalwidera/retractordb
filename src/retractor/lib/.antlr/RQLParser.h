
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, BYTE_T = 5, STRING_T = 6, UNSIGNED_T = 7, 
    INTEGER_T = 8, FLOAT_T = 9, DOUBLE_T = 10, RATIONAL_T = 11, SELECT = 12, 
    STREAM = 13, FROM = 14, DECLARE = 15, RETENTION = 16, FILE = 17, BINFILE = 18, 
    TEXTFILE = 19, DEVICE = 20, STORAGE = 21, ROTATION = 22, SUBSTRAT = 23, 
    RULE = 24, DISPOSABLE = 25, ONESHOT = 26, HOLD = 27, TIMEOUT = 28, VOLATILE = 29, 
    PERSISTENT = 30, DEFAULT = 31, LET = 32, VAR = 33, ON = 34, WHEN = 35, 
    DUMP = 36, SYSTEM = 37, SET = 38, DO = 39, TO = 40, AND_C = 41, OR_C = 42, 
    NOT_C = 43, MIN = 44, MAX = 45, AVG = 46, SUMC = 47, TYPE_PROFILE = 48, 
    STRING_PROFILE = 49, ID = 50, CONST_REF = 51, STRING = 52, FLOAT = 53, 
    DECIMAL = 54, REAL = 55, IS_EQ = 56, IS_NQ = 57, IS_GR = 58, IS_LS = 59, 
    IS_GE = 60, IS_LE = 61, EXCLAMATION = 62, DOUBLE_BAR = 63, DOT = 64, 
    UNDERLINE = 65, AT = 66, SHARP = 67, AND = 68, MOD = 69, DOLLAR = 70, 
    COMMA = 71, SEMI = 72, COLON = 73, DOUBLE_COLON = 74, STAR = 75, DIVIDE = 76, 
    PLUS = 77, MINUS = 78, BIT_NOT = 79, BIT_OR = 80, BIT_XOR = 81, SPACE = 82, 
    COMMENT = 83, LINE_COMMENT2 = 84
  };

  enum {
    RuleProg = 0, RuleDefault_statement = 1, RuleLet_statement = 2, RuleLet_value = 3, 
    RuleVar_statement = 4, RuleVar_type = 5, RuleInt_const = 6, RuleCompiler_option = 7, 
    RuleSelect_statement = 8, RuleDeclare_statement = 9, RuleRule_statement = 10, 
    RuleDumppart = 11, RuleSystempart = 12, RuleSetpart = 13, RuleRational_se = 14, 
    RuleRetention_from = 15, RuleFraction_rule = 16, RuleField_declaration = 17, 
    RuleField_type = 18, RuleSelect_list = 19, RuleField_id = 20, RuleUnary_op_expression = 21, 
    RuleAsterisk = 22, RuleExpression = 23, RuleLogic = 24, RuleExpression_logic = 25, 
    RuleTerm_logic = 26, RuleExpression_factor = 27, RuleTerm = 28, RuleStream_expression = 29, 
    RuleStream_factor = 30, RuleGen_index = 31, RuleAgregator = 32, RuleStream_fn_call = 33, 
    RuleWindow_agg = 34, RuleFunction_call = 35
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
  class Default_statementContext;
  class Let_statementContext;
  class Let_valueContext;
  class Var_statementContext;
  class Var_typeContext;
  class Int_constContext;
  class Compiler_optionContext;
  class Select_statementContext;
  class Declare_statementContext;
  class Rule_statementContext;
  class DumppartContext;
  class SystempartContext;
  class SetpartContext;
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
  class Stream_factorContext;
  class Gen_indexContext;
  class AgregatorContext;
  class Stream_fn_callContext;
  class Window_aggContext;
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
    std::vector<Compiler_optionContext *> compiler_option();
    Compiler_optionContext* compiler_option(size_t i);
    std::vector<Default_statementContext *> default_statement();
    Default_statementContext* default_statement(size_t i);
    std::vector<Rule_statementContext *> rule_statement();
    Rule_statementContext* rule_statement(size_t i);
    std::vector<Let_statementContext *> let_statement();
    Let_statementContext* let_statement(size_t i);
    std::vector<Var_statementContext *> var_statement();
    Var_statementContext* var_statement(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  ProgContext* prog();

  class  Default_statementContext : public antlr4::ParserRuleContext {
  public:
    Default_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Default_statementContext() = default;
    void copyFrom(Default_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  DefaultOptionContext : public Default_statementContext {
  public:
    DefaultOptionContext(Default_statementContext *ctx);

    antlr4::tree::TerminalNode *DEFAULT();
    antlr4::tree::TerminalNode *VOLATILE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Default_statementContext* default_statement();

  class  Let_statementContext : public antlr4::ParserRuleContext {
  public:
    Let_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Let_statementContext() = default;
    void copyFrom(Let_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  LetContext : public Let_statementContext {
  public:
    LetContext(Let_statementContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::tree::TerminalNode *LET();
    antlr4::tree::TerminalNode *IS_EQ();
    Let_valueContext *let_value();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *COLON();
    Var_typeContext *var_type();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Let_statementContext* let_statement();

  class  Let_valueContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *sign = nullptr;
    antlr4::Token *nom = nullptr;
    antlr4::Token *den = nullptr;
    Let_valueContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *DIVIDE();
    antlr4::tree::TerminalNode *MINUS();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Let_valueContext* let_value();

  class  Var_statementContext : public antlr4::ParserRuleContext {
  public:
    Var_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Var_statementContext() = default;
    void copyFrom(Var_statementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  VarContext : public Var_statementContext {
  public:
    VarContext(Var_statementContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::tree::TerminalNode *VAR();
    antlr4::tree::TerminalNode *IS_EQ();
    Let_valueContext *let_value();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *COLON();
    Var_typeContext *var_type();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Var_statementContext* var_statement();

  class  Var_typeContext : public antlr4::ParserRuleContext {
  public:
    Var_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INTEGER_T();
    antlr4::tree::TerminalNode *UNSIGNED_T();
    antlr4::tree::TerminalNode *FLOAT_T();
    antlr4::tree::TerminalNode *DOUBLE_T();
    antlr4::tree::TerminalNode *RATIONAL_T();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Var_typeContext* var_type();

  class  Int_constContext : public antlr4::ParserRuleContext {
  public:
    Int_constContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *CONST_REF();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Int_constContext* int_const();

  class  Compiler_optionContext : public antlr4::ParserRuleContext {
  public:
    Compiler_optionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Compiler_optionContext() = default;
    void copyFrom(Compiler_optionContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  CoptionContext : public Compiler_optionContext {
  public:
    CoptionContext(Compiler_optionContext *ctx);

    antlr4::Token *directive = nullptr;
    antlr4::Token *value = nullptr;
    antlr4::tree::TerminalNode *ROTATION();
    antlr4::tree::TerminalNode *STORAGE();
    antlr4::tree::TerminalNode *SUBSTRAT();
    antlr4::tree::TerminalNode *STRING_PROFILE();
    antlr4::tree::TerminalNode *STRING();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Compiler_optionContext* compiler_option();

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
    RQLParser::Int_constContext *gen_size = nullptr;
    antlr4::Token *file_name = nullptr;
    antlr4::Token *type_name = nullptr;
    antlr4::tree::TerminalNode *SELECT();
    Select_listContext *select_list();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *FROM();
    Stream_expressionContext *stream_expression();
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    antlr4::tree::TerminalNode *FILE();
    Retention_fromContext *retention_from();
    antlr4::tree::TerminalNode *STORAGE();
    Int_constContext *int_const();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *VOLATILE();
    antlr4::tree::TerminalNode *PERSISTENT();
    antlr4::tree::TerminalNode *TYPE_PROFILE();
    antlr4::tree::TerminalNode *DEFAULT();
    antlr4::tree::TerminalNode *DEVICE();
    antlr4::tree::TerminalNode *BINFILE();
    antlr4::tree::TerminalNode *TEXTFILE();
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
    antlr4::Token *kind = nullptr;
    antlr4::Token *file_name = nullptr;
    antlr4::Token *timeout_sign = nullptr;
    RQLParser::Rational_seContext *timeout_value = nullptr;
    antlr4::tree::TerminalNode *DECLARE();
    std::vector<Field_declarationContext *> field_declaration();
    Field_declarationContext* field_declaration(size_t i);
    antlr4::tree::TerminalNode *STREAM();
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);
    std::vector<Rational_seContext *> rational_se();
    Rational_seContext* rational_se(size_t i);
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *BINFILE();
    antlr4::tree::TerminalNode *TEXTFILE();
    antlr4::tree::TerminalNode *DEVICE();
    antlr4::tree::TerminalNode *FILE();
    antlr4::tree::TerminalNode *TIMEOUT();
    antlr4::tree::TerminalNode *DISPOSABLE();
    antlr4::tree::TerminalNode *ONESHOT();
    antlr4::tree::TerminalNode *HOLD();
    antlr4::tree::TerminalNode *MINUS();
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
    SetpartContext *setpart();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Rule_statementContext* rule_statement();

  class  DumppartContext : public antlr4::ParserRuleContext {
  public:
    RQLParser::Int_constContext *step_back = nullptr;
    RQLParser::Int_constContext *step_forward = nullptr;
    RQLParser::Int_constContext *rule_retnetion = nullptr;
    DumppartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DUMP();
    antlr4::tree::TerminalNode *TO();
    std::vector<Int_constContext *> int_const();
    Int_constContext* int_const(size_t i);
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

  class  SetpartContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *target = nullptr;
    RQLParser::Expression_factorContext *value = nullptr;
    SetpartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SET();
    antlr4::tree::TerminalNode *IS_EQ();
    antlr4::tree::TerminalNode *CONST_REF();
    Expression_factorContext *expression_factor();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  SetpartContext* setpart();

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

  class  RationalAsConstContext : public Rational_seContext {
  public:
    RationalAsConstContext(Rational_seContext *ctx);

    antlr4::tree::TerminalNode *CONST_REF();
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

    RQLParser::Int_constContext *capacity = nullptr;
    RQLParser::Int_constContext *segments = nullptr;
    antlr4::tree::TerminalNode *RETENTION();
    std::vector<Int_constContext *> int_const();
    Int_constContext* int_const(size_t i);
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

    RQLParser::Int_constContext *type_size = nullptr;
    antlr4::tree::TerminalNode *ID();
    Field_typeContext *field_type();
    Int_constContext *int_const();
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
    RQLParser::Int_constContext *column_index = nullptr;
    antlr4::tree::TerminalNode *ID();
    Int_constContext *int_const();
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

  class  FieldIDGeneratedContext : public Field_idContext {
  public:
    FieldIDGeneratedContext(Field_idContext *ctx);

    antlr4::Token *tablename = nullptr;
    Gen_indexContext *gen_index();
    antlr4::tree::TerminalNode *ID();
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
    TermContext *term();
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

  class  ExpNotContext : public Expression_logicContext {
  public:
    ExpNotContext(Expression_logicContext *ctx);

    antlr4::tree::TerminalNode *NOT_C();
    Expression_logicContext *expression_logic();
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
    antlr4::Token *op = nullptr;
    Term_logicContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Expression_factorContext *> expression_factor();
    Expression_factorContext* expression_factor(size_t i);
    antlr4::tree::TerminalNode *IS_EQ();
    antlr4::tree::TerminalNode *IS_NQ();
    antlr4::tree::TerminalNode *IS_GR();
    antlr4::tree::TerminalNode *IS_LS();
    antlr4::tree::TerminalNode *IS_GE();
    antlr4::tree::TerminalNode *IS_LE();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Term_logicContext* term_logic();

  class  Expression_factorContext : public antlr4::ParserRuleContext {
  public:
    Expression_factorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    Expression_factorContext() = default;
    void copyFrom(Expression_factorContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpTermContext : public Expression_factorContext {
  public:
    ExpTermContext(Expression_factorContext *ctx);

    TermContext *term();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpAddSubContext : public Expression_factorContext {
  public:
    ExpAddSubContext(Expression_factorContext *ctx);

    antlr4::Token *op = nullptr;
    std::vector<Expression_factorContext *> expression_factor();
    Expression_factorContext* expression_factor(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();
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

  class  ExpDecContext : public TermContext {
  public:
    ExpDecContext(TermContext *ctx);

    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpGenIndexContext : public TermContext {
  public:
    ExpGenIndexContext(TermContext *ctx);

    antlr4::tree::TerminalNode *DOLLAR();
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

  class  ExpMulDivContext : public TermContext {
  public:
    ExpMulDivContext(TermContext *ctx);

    antlr4::Token *op = nullptr;
    std::vector<TermContext *> term();
    TermContext* term(size_t i);
    antlr4::tree::TerminalNode *STAR();
    antlr4::tree::TerminalNode *DIVIDE();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ExpWindowAggContext : public TermContext {
  public:
    ExpWindowAggContext(TermContext *ctx);

    Window_aggContext *window_agg();
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

  class  ExpPowContext : public TermContext {
  public:
    ExpPowContext(TermContext *ctx);

    std::vector<TermContext *> term();
    TermContext* term(size_t i);
    antlr4::tree::TerminalNode *BIT_XOR();
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

  class  ExpConstContext : public TermContext {
  public:
    ExpConstContext(TermContext *ctx);

    antlr4::tree::TerminalNode *CONST_REF();
    antlr4::tree::TerminalNode *MINUS();
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

    std::vector<Stream_expressionContext *> stream_expression();
    Stream_expressionContext* stream_expression(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpTimeMoveContext : public Stream_expressionContext {
  public:
    SExpTimeMoveContext(Stream_expressionContext *ctx);

    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *IS_GR();
    Int_constContext *int_const();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpFactorContext : public Stream_expressionContext {
  public:
    SExpFactorContext(Stream_expressionContext *ctx);

    Stream_factorContext *stream_factor();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpHashContext : public Stream_expressionContext {
  public:
    SExpHashContext(Stream_expressionContext *ctx);

    std::vector<Stream_expressionContext *> stream_expression();
    Stream_expressionContext* stream_expression(size_t i);
    antlr4::tree::TerminalNode *SHARP();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpModContext : public Stream_expressionContext {
  public:
    SExpModContext(Stream_expressionContext *ctx);

    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *MOD();
    Rational_seContext *rational_se();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpAgregate_proformaContext : public Stream_expressionContext {
  public:
    SExpAgregate_proformaContext(Stream_expressionContext *ctx);

    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *DOT();
    AgregatorContext *agregator();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpAgseContext : public Stream_expressionContext {
  public:
    SExpAgseContext(Stream_expressionContext *ctx);

    RQLParser::Int_constContext *step = nullptr;
    RQLParser::Int_constContext *window = nullptr;
    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *AT();
    antlr4::tree::TerminalNode *COMMA();
    std::vector<Int_constContext *> int_const();
    Int_constContext* int_const(size_t i);
    antlr4::tree::TerminalNode *MINUS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpMinusContext : public Stream_expressionContext {
  public:
    SExpMinusContext(Stream_expressionContext *ctx);

    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *MINUS();
    Rational_seContext *rational_se();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  SExpAndContext : public Stream_expressionContext {
  public:
    SExpAndContext(Stream_expressionContext *ctx);

    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *AND();
    Rational_seContext *rational_se();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  Stream_expressionContext* stream_expression();
  Stream_expressionContext* stream_expression(int precedence);
  class  Stream_factorContext : public antlr4::ParserRuleContext {
  public:
    Stream_factorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();
    Gen_indexContext *gen_index();
    Stream_expressionContext *stream_expression();
    Stream_fn_callContext *stream_fn_call();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Stream_factorContext* stream_factor();

  class  Gen_indexContext : public antlr4::ParserRuleContext {
  public:
    Gen_indexContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Gen_indexContext *> gen_index();
    Gen_indexContext* gen_index(size_t i);
    antlr4::tree::TerminalNode *DOLLAR();
    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *CONST_REF();
    antlr4::tree::TerminalNode *STAR();
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Gen_indexContext* gen_index();
  Gen_indexContext* gen_index(int precedence);
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

  class  Stream_fn_callContext : public antlr4::ParserRuleContext {
  public:
    Stream_fn_callContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *MIN();
    antlr4::tree::TerminalNode *MAX();
    antlr4::tree::TerminalNode *AVG();
    antlr4::tree::TerminalNode *SUMC();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Stream_fn_callContext* stream_fn_call();

  class  Window_aggContext : public antlr4::ParserRuleContext {
  public:
    RQLParser::Int_constContext *width = nullptr;
    Window_aggContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Expression_factorContext *expression_factor();
    antlr4::tree::TerminalNode *COLON();
    antlr4::tree::TerminalNode *MIN();
    antlr4::tree::TerminalNode *MAX();
    antlr4::tree::TerminalNode *AVG();
    antlr4::tree::TerminalNode *SUMC();
    Int_constContext *int_const();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Window_aggContext* window_agg();

  class  Function_callContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *fn = nullptr;
    RQLParser::Int_constContext *width = nullptr;
    Function_callContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Expression_factorContext *expression_factor();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *COLON();
    Int_constContext *int_const();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  Function_callContext* function_call();


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool expression_logicSempred(Expression_logicContext *_localctx, size_t predicateIndex);
  bool expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex);
  bool termSempred(TermContext *_localctx, size_t predicateIndex);
  bool stream_expressionSempred(Stream_expressionContext *_localctx, size_t predicateIndex);
  bool gen_indexSempred(Gen_indexContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

