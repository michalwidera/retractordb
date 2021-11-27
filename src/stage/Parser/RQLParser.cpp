
    // https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3


#include "RQLVisitor.h"

#include "RQLParser.h"


using namespace antlrcpp;
using namespace antlr4;

RQLParser::RQLParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

RQLParser::~RQLParser() {
  delete _interpreter;
}

std::string RQLParser::getGrammarFileName() const {
  return "RQL.g4";
}

const std::vector<std::string>& RQLParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& RQLParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- ProgContext ------------------------------------------------------------------

RQLParser::ProgContext::ProgContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::ProgContext::EOF() {
  return getToken(RQLParser::EOF, 0);
}

std::vector<RQLParser::Select_statementContext *> RQLParser::ProgContext::select_statement() {
  return getRuleContexts<RQLParser::Select_statementContext>();
}

RQLParser::Select_statementContext* RQLParser::ProgContext::select_statement(size_t i) {
  return getRuleContext<RQLParser::Select_statementContext>(i);
}

std::vector<RQLParser::Declare_statementContext *> RQLParser::ProgContext::declare_statement() {
  return getRuleContexts<RQLParser::Declare_statementContext>();
}

RQLParser::Declare_statementContext* RQLParser::ProgContext::declare_statement(size_t i) {
  return getRuleContext<RQLParser::Declare_statementContext>(i);
}


size_t RQLParser::ProgContext::getRuleIndex() const {
  return RQLParser::RuleProg;
}


antlrcpp::Any RQLParser::ProgContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitProg(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::ProgContext* RQLParser::prog() {
  ProgContext *_localctx = _tracker.createInstance<ProgContext>(_ctx, getState());
  enterRule(_localctx, 0, RQLParser::RuleProg);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(18); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(18);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(16);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(17);
          declare_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(20); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == RQLParser::SELECT

    || _la == RQLParser::DECLARE);
    setState(22);
    match(RQLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_statementContext ------------------------------------------------------------------

RQLParser::Select_statementContext::Select_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Select_statementContext::SELECT() {
  return getToken(RQLParser::SELECT, 0);
}

tree::TerminalNode* RQLParser::Select_statementContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

tree::TerminalNode* RQLParser::Select_statementContext::FROM() {
  return getToken(RQLParser::FROM, 0);
}

RQLParser::Select_listContext* RQLParser::Select_statementContext::select_list() {
  return getRuleContext<RQLParser::Select_listContext>(0);
}

tree::TerminalNode* RQLParser::Select_statementContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::Stream_expressionContext* RQLParser::Select_statementContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}


size_t RQLParser::Select_statementContext::getRuleIndex() const {
  return RQLParser::RuleSelect_statement;
}


antlrcpp::Any RQLParser::Select_statementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitSelect_statement(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Select_statementContext* RQLParser::select_statement() {
  Select_statementContext *_localctx = _tracker.createInstance<Select_statementContext>(_ctx, getState());
  enterRule(_localctx, 2, RQLParser::RuleSelect_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(24);
    match(RQLParser::SELECT);
    setState(25);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->columns = select_list();
    setState(26);
    match(RQLParser::STREAM);
    setState(27);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->id = match(RQLParser::ID);
    setState(28);
    match(RQLParser::FROM);
    setState(29);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->from = stream_expression();

                          std::cout << "::columns ID:" << (antlrcpp::downCast<Select_statementContext *>(_localctx)->columns != nullptr ? _input->getText(antlrcpp::downCast<Select_statementContext *>(_localctx)->columns->start, antlrcpp::downCast<Select_statementContext *>(_localctx)->columns->stop) : nullptr) << std::endl;
                          std::cout << "::id ID:" << (antlrcpp::downCast<Select_statementContext *>(_localctx)->id != nullptr ? antlrcpp::downCast<Select_statementContext *>(_localctx)->id->getText() : "") << std::endl;
                          std::cout << "::from ID:" << (antlrcpp::downCast<Select_statementContext *>(_localctx)->from != nullptr ? _input->getText(antlrcpp::downCast<Select_statementContext *>(_localctx)->from->start, antlrcpp::downCast<Select_statementContext *>(_localctx)->from->stop) : nullptr) << std::endl;
                          
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Declare_statementContext ------------------------------------------------------------------

RQLParser::Declare_statementContext::Declare_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Declare_statementContext::DECLARE() {
  return getToken(RQLParser::DECLARE, 0);
}

RQLParser::Column_name_listContext* RQLParser::Declare_statementContext::column_name_list() {
  return getRuleContext<RQLParser::Column_name_listContext>(0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

std::vector<tree::TerminalNode *> RQLParser::Declare_statementContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Declare_statementContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

tree::TerminalNode* RQLParser::Declare_statementContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}


size_t RQLParser::Declare_statementContext::getRuleIndex() const {
  return RQLParser::RuleDeclare_statement;
}


antlrcpp::Any RQLParser::Declare_statementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitDeclare_statement(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Declare_statementContext* RQLParser::declare_statement() {
  Declare_statementContext *_localctx = _tracker.createInstance<Declare_statementContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleDeclare_statement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(32);
    match(RQLParser::DECLARE);
    setState(33);
    column_name_list();
    setState(34);
    match(RQLParser::STREAM);
    setState(35);
    match(RQLParser::ID);
    setState(38);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(36);
      match(RQLParser::FILE);
      setState(37);
      match(RQLParser::ID);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_name_listContext ------------------------------------------------------------------

RQLParser::Column_name_listContext::Column_name_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Column_name_listContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Column_name_listContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<tree::TerminalNode *> RQLParser::Column_name_listContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Column_name_listContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}


size_t RQLParser::Column_name_listContext::getRuleIndex() const {
  return RQLParser::RuleColumn_name_list;
}


antlrcpp::Any RQLParser::Column_name_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitColumn_name_list(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Column_name_listContext* RQLParser::column_name_list() {
  Column_name_listContext *_localctx = _tracker.createInstance<Column_name_listContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleColumn_name_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(40);
    antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken = match(RQLParser::ID);
    antlrcpp::downCast<Column_name_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken);
    setState(45);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(41);
      match(RQLParser::COMMA);
      setState(42);
      antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken = match(RQLParser::ID);
      antlrcpp::downCast<Column_name_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken);
      setState(47);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_listContext ------------------------------------------------------------------

RQLParser::Select_listContext::Select_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Select_listContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Select_listContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<tree::TerminalNode *> RQLParser::Select_listContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Select_listContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}


size_t RQLParser::Select_listContext::getRuleIndex() const {
  return RQLParser::RuleSelect_list;
}


antlrcpp::Any RQLParser::Select_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitSelect_list(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Select_listContext* RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(48);
    antlrcpp::downCast<Select_listContext *>(_localctx)->idToken = match(RQLParser::ID);
    antlrcpp::downCast<Select_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Select_listContext *>(_localctx)->idToken);
    setState(53);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(49);
      match(RQLParser::COMMA);
      setState(50);
      antlrcpp::downCast<Select_listContext *>(_localctx)->idToken = match(RQLParser::ID);
      antlrcpp::downCast<Select_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Select_listContext *>(_localctx)->idToken);
      setState(55);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Stream_expressionContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext::Stream_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Stream_termContext *> RQLParser::Stream_expressionContext::stream_term() {
  return getRuleContexts<RQLParser::Stream_termContext>();
}

RQLParser::Stream_termContext* RQLParser::Stream_expressionContext::stream_term(size_t i) {
  return getRuleContext<RQLParser::Stream_termContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::GREATER() {
  return getTokens(RQLParser::GREATER);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::GREATER(size_t i) {
  return getToken(RQLParser::GREATER, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::PLUS() {
  return getTokens(RQLParser::PLUS);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::PLUS(size_t i) {
  return getToken(RQLParser::PLUS, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::MINUS() {
  return getTokens(RQLParser::MINUS);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::MINUS(size_t i) {
  return getToken(RQLParser::MINUS, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::RATIONAL() {
  return getTokens(RQLParser::RATIONAL);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::RATIONAL(size_t i) {
  return getToken(RQLParser::RATIONAL, i);
}


size_t RQLParser::Stream_expressionContext::getRuleIndex() const {
  return RQLParser::RuleStream_expression;
}


antlrcpp::Any RQLParser::Stream_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitStream_expression(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
  Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleStream_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(56);
    stream_term();
    setState(65);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::GREATER)
      | (1ULL << RQLParser::PLUS)
      | (1ULL << RQLParser::MINUS))) != 0)) {
      setState(63);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::GREATER: {
          setState(57);
          match(RQLParser::GREATER);
          setState(58);
          match(RQLParser::DECIMAL);
          break;
        }

        case RQLParser::PLUS: {
          setState(59);
          match(RQLParser::PLUS);
          setState(60);
          stream_term();
          break;
        }

        case RQLParser::MINUS: {
          setState(61);
          match(RQLParser::MINUS);
          setState(62);
          match(RQLParser::RATIONAL);
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(67);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Stream_termContext ------------------------------------------------------------------

RQLParser::Stream_termContext::Stream_termContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Stream_termContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::SHARP() {
  return getTokens(RQLParser::SHARP);
}

tree::TerminalNode* RQLParser::Stream_termContext::SHARP(size_t i) {
  return getToken(RQLParser::SHARP, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::AND() {
  return getTokens(RQLParser::AND);
}

tree::TerminalNode* RQLParser::Stream_termContext::AND(size_t i) {
  return getToken(RQLParser::AND, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::RATIONAL() {
  return getTokens(RQLParser::RATIONAL);
}

tree::TerminalNode* RQLParser::Stream_termContext::RATIONAL(size_t i) {
  return getToken(RQLParser::RATIONAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::MOD() {
  return getTokens(RQLParser::MOD);
}

tree::TerminalNode* RQLParser::Stream_termContext::MOD(size_t i) {
  return getToken(RQLParser::MOD, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::AT() {
  return getTokens(RQLParser::AT);
}

tree::TerminalNode* RQLParser::Stream_termContext::AT(size_t i) {
  return getToken(RQLParser::AT, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::LR_BRACKET() {
  return getTokens(RQLParser::LR_BRACKET);
}

tree::TerminalNode* RQLParser::Stream_termContext::LR_BRACKET(size_t i) {
  return getToken(RQLParser::LR_BRACKET, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::Stream_termContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Stream_termContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::SIGNED_DECIMAL() {
  return getTokens(RQLParser::SIGNED_DECIMAL);
}

tree::TerminalNode* RQLParser::Stream_termContext::SIGNED_DECIMAL(size_t i) {
  return getToken(RQLParser::SIGNED_DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::RR_BRACKET() {
  return getTokens(RQLParser::RR_BRACKET);
}

tree::TerminalNode* RQLParser::Stream_termContext::RR_BRACKET(size_t i) {
  return getToken(RQLParser::RR_BRACKET, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::DOT() {
  return getTokens(RQLParser::DOT);
}

tree::TerminalNode* RQLParser::Stream_termContext::DOT(size_t i) {
  return getToken(RQLParser::DOT, i);
}

std::vector<RQLParser::AgregatorContext *> RQLParser::Stream_termContext::agregator() {
  return getRuleContexts<RQLParser::AgregatorContext>();
}

RQLParser::AgregatorContext* RQLParser::Stream_termContext::agregator(size_t i) {
  return getRuleContext<RQLParser::AgregatorContext>(i);
}


size_t RQLParser::Stream_termContext::getRuleIndex() const {
  return RQLParser::RuleStream_term;
}


antlrcpp::Any RQLParser::Stream_termContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitStream_term(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Stream_termContext* RQLParser::stream_term() {
  Stream_termContext *_localctx = _tracker.createInstance<Stream_termContext>(_ctx, getState());
  enterRule(_localctx, 12, RQLParser::RuleStream_term);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(68);
    match(RQLParser::ID);
    setState(85);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::DOT)
      | (1ULL << RQLParser::AT)
      | (1ULL << RQLParser::SHARP)
      | (1ULL << RQLParser::AND)
      | (1ULL << RQLParser::MOD))) != 0)) {
      setState(83);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SHARP: {
          setState(69);
          match(RQLParser::SHARP);
          setState(70);
          match(RQLParser::ID);
          break;
        }

        case RQLParser::AND: {
          setState(71);
          match(RQLParser::AND);
          setState(72);
          match(RQLParser::RATIONAL);
          break;
        }

        case RQLParser::MOD: {
          setState(73);
          match(RQLParser::MOD);
          setState(74);
          match(RQLParser::RATIONAL);
          break;
        }

        case RQLParser::AT: {
          setState(75);
          match(RQLParser::AT);
          setState(76);
          match(RQLParser::LR_BRACKET);
          setState(77);
          match(RQLParser::DECIMAL);
          setState(78);
          match(RQLParser::COMMA);
          setState(79);
          match(RQLParser::SIGNED_DECIMAL);
          setState(80);
          match(RQLParser::RR_BRACKET);
          break;
        }

        case RQLParser::DOT: {
          setState(81);
          match(RQLParser::DOT);
          setState(82);
          agregator();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(87);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AgregatorContext ------------------------------------------------------------------

RQLParser::AgregatorContext::AgregatorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::AgregatorContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

tree::TerminalNode* RQLParser::AgregatorContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

tree::TerminalNode* RQLParser::AgregatorContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

tree::TerminalNode* RQLParser::AgregatorContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}


size_t RQLParser::AgregatorContext::getRuleIndex() const {
  return RQLParser::RuleAgregator;
}


antlrcpp::Any RQLParser::AgregatorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitAgregator(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::AgregatorContext* RQLParser::agregator() {
  AgregatorContext *_localctx = _tracker.createInstance<AgregatorContext>(_ctx, getState());
  enterRule(_localctx, 14, RQLParser::RuleAgregator);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(88);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::MIN)
      | (1ULL << RQLParser::MAX)
      | (1ULL << RQLParser::AVG)
      | (1ULL << RQLParser::SUMC))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> RQLParser::_decisionToDFA;
atn::PredictionContextCache RQLParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN RQLParser::_atn;
std::vector<uint16_t> RQLParser::_serializedATN;

std::vector<std::string> RQLParser::_ruleNames = {
  "prog", "select_statement", "declare_statement", "column_name_list", "select_list", 
  "stream_expression", "stream_term", "agregator"
};

std::vector<std::string> RQLParser::_literalNames = {
  "", "'SELECT'", "'STREAM'", "'FROM'", "'DECLARE'", "'FILE'", "'MIN'", 
  "'MAX'", "'AVG'", "'SUMC'", "", "", "", "", "", "", "", "'='", "'>'", 
  "'<'", "'!'", "'||'", "'.'", "'_'", "'@'", "'#'", "'&'", "'%'", "'$'", 
  "'('", "')'", "','", "';'", "':'", "'::'", "'*'", "'/'", "'+'", "'-'", 
  "'~'", "'|'", "'^'"
};

std::vector<std::string> RQLParser::_symbolicNames = {
  "", "SELECT", "STREAM", "FROM", "DECLARE", "FILE", "MIN", "MAX", "AVG", 
  "SUMC", "ID", "STRING", "FLOAT", "DECIMAL", "SIGNED_DECIMAL", "RATIONAL", 
  "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION", "DOUBLE_BAR", "DOT", 
  "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", "LR_BRACKET", "RR_BRACKET", 
  "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", "MINUS", 
  "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT"
};

dfa::Vocabulary RQLParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> RQLParser::_tokenNames;

RQLParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  static const uint16_t serializedATNSegment0[] = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
       0x3, 0x2e, 0x5d, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x3, 0x2, 0x3, 0x2, 
       0x6, 0x2, 0x15, 0xa, 0x2, 0xd, 0x2, 0xe, 0x2, 0x16, 0x3, 0x2, 0x3, 
       0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 
       0x4, 0x3, 0x4, 0x5, 0x4, 0x29, 0xa, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 
       0x5, 0x7, 0x5, 0x2e, 0xa, 0x5, 0xc, 0x5, 0xe, 0x5, 0x31, 0xb, 0x5, 
       0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x7, 0x6, 0x36, 0xa, 0x6, 0xc, 0x6, 
       0xe, 0x6, 0x39, 0xb, 0x6, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 
       0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x7, 0x7, 0x42, 0xa, 0x7, 0xc, 0x7, 
       0xe, 0x7, 0x45, 0xb, 0x7, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 
       0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 
       0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x7, 0x8, 0x56, 0xa, 
       0x8, 0xc, 0x8, 0xe, 0x8, 0x59, 0xb, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 
       0x9, 0x2, 0x2, 0xa, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x2, 
       0x3, 0x3, 0x2, 0x8, 0xb, 0x2, 0x61, 0x2, 0x14, 0x3, 0x2, 0x2, 0x2, 
       0x4, 0x1a, 0x3, 0x2, 0x2, 0x2, 0x6, 0x22, 0x3, 0x2, 0x2, 0x2, 0x8, 
       0x2a, 0x3, 0x2, 0x2, 0x2, 0xa, 0x32, 0x3, 0x2, 0x2, 0x2, 0xc, 0x3a, 
       0x3, 0x2, 0x2, 0x2, 0xe, 0x46, 0x3, 0x2, 0x2, 0x2, 0x10, 0x5a, 0x3, 
       0x2, 0x2, 0x2, 0x12, 0x15, 0x5, 0x4, 0x3, 0x2, 0x13, 0x15, 0x5, 0x6, 
       0x4, 0x2, 0x14, 0x12, 0x3, 0x2, 0x2, 0x2, 0x14, 0x13, 0x3, 0x2, 0x2, 
       0x2, 0x15, 0x16, 0x3, 0x2, 0x2, 0x2, 0x16, 0x14, 0x3, 0x2, 0x2, 0x2, 
       0x16, 0x17, 0x3, 0x2, 0x2, 0x2, 0x17, 0x18, 0x3, 0x2, 0x2, 0x2, 0x18, 
       0x19, 0x7, 0x2, 0x2, 0x3, 0x19, 0x3, 0x3, 0x2, 0x2, 0x2, 0x1a, 0x1b, 
       0x7, 0x3, 0x2, 0x2, 0x1b, 0x1c, 0x5, 0xa, 0x6, 0x2, 0x1c, 0x1d, 0x7, 
       0x4, 0x2, 0x2, 0x1d, 0x1e, 0x7, 0xc, 0x2, 0x2, 0x1e, 0x1f, 0x7, 0x5, 
       0x2, 0x2, 0x1f, 0x20, 0x5, 0xc, 0x7, 0x2, 0x20, 0x21, 0x8, 0x3, 0x1, 
       0x2, 0x21, 0x5, 0x3, 0x2, 0x2, 0x2, 0x22, 0x23, 0x7, 0x6, 0x2, 0x2, 
       0x23, 0x24, 0x5, 0x8, 0x5, 0x2, 0x24, 0x25, 0x7, 0x4, 0x2, 0x2, 0x25, 
       0x28, 0x7, 0xc, 0x2, 0x2, 0x26, 0x27, 0x7, 0x7, 0x2, 0x2, 0x27, 0x29, 
       0x7, 0xc, 0x2, 0x2, 0x28, 0x26, 0x3, 0x2, 0x2, 0x2, 0x28, 0x29, 0x3, 
       0x2, 0x2, 0x2, 0x29, 0x7, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x2f, 0x7, 0xc, 
       0x2, 0x2, 0x2b, 0x2c, 0x7, 0x21, 0x2, 0x2, 0x2c, 0x2e, 0x7, 0xc, 
       0x2, 0x2, 0x2d, 0x2b, 0x3, 0x2, 0x2, 0x2, 0x2e, 0x31, 0x3, 0x2, 0x2, 
       0x2, 0x2f, 0x2d, 0x3, 0x2, 0x2, 0x2, 0x2f, 0x30, 0x3, 0x2, 0x2, 0x2, 
       0x30, 0x9, 0x3, 0x2, 0x2, 0x2, 0x31, 0x2f, 0x3, 0x2, 0x2, 0x2, 0x32, 
       0x37, 0x7, 0xc, 0x2, 0x2, 0x33, 0x34, 0x7, 0x21, 0x2, 0x2, 0x34, 
       0x36, 0x7, 0xc, 0x2, 0x2, 0x35, 0x33, 0x3, 0x2, 0x2, 0x2, 0x36, 0x39, 
       0x3, 0x2, 0x2, 0x2, 0x37, 0x35, 0x3, 0x2, 0x2, 0x2, 0x37, 0x38, 0x3, 
       0x2, 0x2, 0x2, 0x38, 0xb, 0x3, 0x2, 0x2, 0x2, 0x39, 0x37, 0x3, 0x2, 
       0x2, 0x2, 0x3a, 0x43, 0x5, 0xe, 0x8, 0x2, 0x3b, 0x3c, 0x7, 0x14, 
       0x2, 0x2, 0x3c, 0x42, 0x7, 0xf, 0x2, 0x2, 0x3d, 0x3e, 0x7, 0x27, 
       0x2, 0x2, 0x3e, 0x42, 0x5, 0xe, 0x8, 0x2, 0x3f, 0x40, 0x7, 0x28, 
       0x2, 0x2, 0x40, 0x42, 0x7, 0x11, 0x2, 0x2, 0x41, 0x3b, 0x3, 0x2, 
       0x2, 0x2, 0x41, 0x3d, 0x3, 0x2, 0x2, 0x2, 0x41, 0x3f, 0x3, 0x2, 0x2, 
       0x2, 0x42, 0x45, 0x3, 0x2, 0x2, 0x2, 0x43, 0x41, 0x3, 0x2, 0x2, 0x2, 
       0x43, 0x44, 0x3, 0x2, 0x2, 0x2, 0x44, 0xd, 0x3, 0x2, 0x2, 0x2, 0x45, 
       0x43, 0x3, 0x2, 0x2, 0x2, 0x46, 0x57, 0x7, 0xc, 0x2, 0x2, 0x47, 0x48, 
       0x7, 0x1b, 0x2, 0x2, 0x48, 0x56, 0x7, 0xc, 0x2, 0x2, 0x49, 0x4a, 
       0x7, 0x1c, 0x2, 0x2, 0x4a, 0x56, 0x7, 0x11, 0x2, 0x2, 0x4b, 0x4c, 
       0x7, 0x1d, 0x2, 0x2, 0x4c, 0x56, 0x7, 0x11, 0x2, 0x2, 0x4d, 0x4e, 
       0x7, 0x1a, 0x2, 0x2, 0x4e, 0x4f, 0x7, 0x1f, 0x2, 0x2, 0x4f, 0x50, 
       0x7, 0xf, 0x2, 0x2, 0x50, 0x51, 0x7, 0x21, 0x2, 0x2, 0x51, 0x52, 
       0x7, 0x10, 0x2, 0x2, 0x52, 0x56, 0x7, 0x20, 0x2, 0x2, 0x53, 0x54, 
       0x7, 0x18, 0x2, 0x2, 0x54, 0x56, 0x5, 0x10, 0x9, 0x2, 0x55, 0x47, 
       0x3, 0x2, 0x2, 0x2, 0x55, 0x49, 0x3, 0x2, 0x2, 0x2, 0x55, 0x4b, 0x3, 
       0x2, 0x2, 0x2, 0x55, 0x4d, 0x3, 0x2, 0x2, 0x2, 0x55, 0x53, 0x3, 0x2, 
       0x2, 0x2, 0x56, 0x59, 0x3, 0x2, 0x2, 0x2, 0x57, 0x55, 0x3, 0x2, 0x2, 
       0x2, 0x57, 0x58, 0x3, 0x2, 0x2, 0x2, 0x58, 0xf, 0x3, 0x2, 0x2, 0x2, 
       0x59, 0x57, 0x3, 0x2, 0x2, 0x2, 0x5a, 0x5b, 0x9, 0x2, 0x2, 0x2, 0x5b, 
       0x11, 0x3, 0x2, 0x2, 0x2, 0xb, 0x14, 0x16, 0x28, 0x2f, 0x37, 0x41, 
       0x43, 0x55, 0x57, 
  };

  _serializedATN.insert(_serializedATN.end(), serializedATNSegment0,
    serializedATNSegment0 + sizeof(serializedATNSegment0) / sizeof(serializedATNSegment0[0]));


  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

RQLParser::Initializer RQLParser::_init;
