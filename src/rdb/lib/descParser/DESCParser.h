
// Generated from DESC.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  DESCParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, BYTE_T = 6, STRING_T = 7, 
    UNSIGNED_T = 8, INTEGER_T = 9, FLOAT_T = 10, DOUBLE_T = 11, RATIONAL_T = 12, 
    INTPAIR_T = 13, IDXPAIR_T = 14, TYPE_T = 15, REF_T = 16, ID = 17, STRING = 18, 
    DECIMAL = 19, REF_TYPE_ARG = 20
  };

  enum {
    RuleDesc = 0, RuleCommand = 1
  };

  explicit DESCParser(antlr4::TokenStream *input);

  DESCParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~DESCParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class DescContext;
  class CommandContext; 

  class  DescContext : public antlr4::ParserRuleContext {
  public:
    DescContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<CommandContext *> command();
    CommandContext* command(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  DescContext* desc();

  class  CommandContext : public antlr4::ParserRuleContext {
  public:
    CommandContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    CommandContext() = default;
    void copyFrom(CommandContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  UnsignedIDContext : public CommandContext {
  public:
    UnsignedIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *arr = nullptr;
    antlr4::tree::TerminalNode *UNSIGNED_T();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  TypeIDContext : public CommandContext {
  public:
    TypeIDContext(CommandContext *ctx);

    antlr4::Token *type = nullptr;
    antlr4::tree::TerminalNode *TYPE_T();
    antlr4::tree::TerminalNode *REF_TYPE_ARG();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  StringIDContext : public CommandContext {
  public:
    StringIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::tree::TerminalNode *STRING_T();
    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *ID();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  IntegerIDContext : public CommandContext {
  public:
    IntegerIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *arr = nullptr;
    antlr4::tree::TerminalNode *INTEGER_T();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ByteIDContext : public CommandContext {
  public:
    ByteIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *arr = nullptr;
    antlr4::tree::TerminalNode *BYTE_T();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  RefIDContext : public CommandContext {
  public:
    RefIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *file = nullptr;
    antlr4::tree::TerminalNode *REF_T();
    std::vector<antlr4::tree::TerminalNode *> STRING();
    antlr4::tree::TerminalNode* STRING(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  FloatIDContext : public CommandContext {
  public:
    FloatIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *arr = nullptr;
    antlr4::tree::TerminalNode *FLOAT_T();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  DoubleIDContext : public CommandContext {
  public:
    DoubleIDContext(CommandContext *ctx);

    antlr4::Token *name = nullptr;
    antlr4::Token *arr = nullptr;
    antlr4::tree::TerminalNode *DOUBLE_T();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DECIMAL();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  CommandContext* command();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

