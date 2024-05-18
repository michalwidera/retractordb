
// Generated from DESC.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  DESCParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, BYTE_T = 3, STRING_T = 4, UNSIGNED_T = 5, INTEGER_T = 6, 
    FLOAT_T = 7, DOUBLE_T = 8, RATIONAL_T = 9, INTPAIR_T = 10, IDXPAIR_T = 11, 
    TYPE_T = 12, REF_T = 13, ID = 14, STRING = 15, REF_TYPE_ARG = 16
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
    antlr4::Token *name = nullptr;
    CommandContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BYTE_T();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *STRING_T();
    antlr4::tree::TerminalNode *UNSIGNED_T();
    antlr4::tree::TerminalNode *FLOAT_T();
    antlr4::tree::TerminalNode *DOUBLE_T();
    antlr4::tree::TerminalNode *REF_T();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *TYPE_T();
    antlr4::tree::TerminalNode *REF_TYPE_ARG();

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

