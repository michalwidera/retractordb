
// Generated from RQLParser.g4 by ANTLR 4.9.3


#include "RQLParserVisitor.h"

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
  return "RQLParser.g4";
}

const std::vector<std::string>& RQLParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& RQLParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- ClausesContext ------------------------------------------------------------------

RQLParser::ClausesContext::ClausesContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Select_statementContext* RQLParser::ClausesContext::select_statement() {
  return getRuleContext<RQLParser::Select_statementContext>(0);
}

RQLParser::Declare_statementContext* RQLParser::ClausesContext::declare_statement() {
  return getRuleContext<RQLParser::Declare_statementContext>(0);
}


size_t RQLParser::ClausesContext::getRuleIndex() const {
  return RQLParser::RuleClauses;
}


antlrcpp::Any RQLParser::ClausesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLParserVisitor*>(visitor))
    return parserVisitor->visitClauses(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::ClausesContext* RQLParser::clauses() {
  ClausesContext *_localctx = _tracker.createInstance<ClausesContext>(_ctx, getState());
  enterRule(_localctx, 0, RQLParser::RuleClauses);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(14);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::SELECT: {
        enterOuterAlt(_localctx, 1);
        setState(12);
        select_statement();
        break;
      }

      case RQLParser::DECLARE: {
        enterOuterAlt(_localctx, 2);
        setState(13);
        declare_statement();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
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

tree::TerminalNode* RQLParser::Select_statementContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::Select_statementContext::FROM() {
  return getToken(RQLParser::FROM, 0);
}

RQLParser::Select_listContext* RQLParser::Select_statementContext::select_list() {
  return getRuleContext<RQLParser::Select_listContext>(0);
}

RQLParser::Stream_expressionContext* RQLParser::Select_statementContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}


size_t RQLParser::Select_statementContext::getRuleIndex() const {
  return RQLParser::RuleSelect_statement;
}


antlrcpp::Any RQLParser::Select_statementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLParserVisitor*>(visitor))
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
    setState(16);
    match(RQLParser::SELECT);
    setState(17);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->columns = select_list();
    setState(18);
    match(RQLParser::STREAM);
    setState(19);
    match(RQLParser::ID);
    setState(20);
    match(RQLParser::FROM);
    setState(21);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->from = stream_expression();
   
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

RQLParser::Full_column_name_listContext* RQLParser::Declare_statementContext::full_column_name_list() {
  return getRuleContext<RQLParser::Full_column_name_listContext>(0);
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
  if (auto parserVisitor = dynamic_cast<RQLParserVisitor*>(visitor))
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
    setState(23);
    match(RQLParser::DECLARE);
    setState(24);
    full_column_name_list();
    setState(25);
    match(RQLParser::STREAM);
    setState(26);
    match(RQLParser::ID);
    setState(29);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(27);
      match(RQLParser::FILE);
      setState(28);
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

//----------------- Full_column_name_listContext ------------------------------------------------------------------

RQLParser::Full_column_name_listContext::Full_column_name_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Full_column_name_listContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Full_column_name_listContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<tree::TerminalNode *> RQLParser::Full_column_name_listContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Full_column_name_listContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}


size_t RQLParser::Full_column_name_listContext::getRuleIndex() const {
  return RQLParser::RuleFull_column_name_list;
}


antlrcpp::Any RQLParser::Full_column_name_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLParserVisitor*>(visitor))
    return parserVisitor->visitFull_column_name_list(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Full_column_name_listContext* RQLParser::full_column_name_list() {
  Full_column_name_listContext *_localctx = _tracker.createInstance<Full_column_name_listContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleFull_column_name_list);
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
    setState(31);
    antlrcpp::downCast<Full_column_name_listContext *>(_localctx)->idToken = match(RQLParser::ID);
    antlrcpp::downCast<Full_column_name_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Full_column_name_listContext *>(_localctx)->idToken);
    setState(36);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(32);
      match(RQLParser::COMMA);
      setState(33);
      antlrcpp::downCast<Full_column_name_listContext *>(_localctx)->idToken = match(RQLParser::ID);
      antlrcpp::downCast<Full_column_name_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Full_column_name_listContext *>(_localctx)->idToken);
      setState(38);
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

tree::TerminalNode* RQLParser::Select_listContext::ID() {
  return getToken(RQLParser::ID, 0);
}


size_t RQLParser::Select_listContext::getRuleIndex() const {
  return RQLParser::RuleSelect_list;
}


antlrcpp::Any RQLParser::Select_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLParserVisitor*>(visitor))
    return parserVisitor->visitSelect_list(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Select_listContext* RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleSelect_list);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(39);
    match(RQLParser::ID);
   
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

tree::TerminalNode* RQLParser::Stream_expressionContext::ID() {
  return getToken(RQLParser::ID, 0);
}


size_t RQLParser::Stream_expressionContext::getRuleIndex() const {
  return RQLParser::RuleStream_expression;
}


antlrcpp::Any RQLParser::Stream_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLParserVisitor*>(visitor))
    return parserVisitor->visitStream_expression(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
  Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleStream_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(41);
    match(RQLParser::ID);
   
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
  "clauses", "select_statement", "declare_statement", "full_column_name_list", 
  "select_list", "stream_expression"
};

std::vector<std::string> RQLParser::_literalNames = {
  "", "'SELECT'", "'STREAM'", "'FROM'", "'DECLARE'", "'FILE'", "", "", "", 
  "", "", "'='", "'>'", "'<'", "'!'", "'||'", "'.'", "'_'", "'@'", "'#'", 
  "'$'", "'('", "')'", "','", "';'", "':'", "'::'", "'*'", "'/'", "'%'", 
  "'+'", "'-'", "'~'", "'|'", "'&'", "'^'"
};

std::vector<std::string> RQLParser::_symbolicNames = {
  "", "SELECT", "STREAM", "FROM", "DECLARE", "FILE", "ID", "STRING", "FLOAT", 
  "DECIMAL", "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION", "DOUBLE_BAR", 
  "DOT", "UNDERLINE", "AT", "SHARP", "DOLLAR", "LR_BRACKET", "RR_BRACKET", 
  "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "MODULE", 
  "PLUS", "MINUS", "BIT_NOT", "BIT_OR", "BIT_AND", "BIT_XOR", "SPACE", "COMMENT", 
  "LINE_COMMENT"
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
       0x3, 0x28, 0x2e, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x3, 0x2, 0x3, 0x2, 0x5, 0x2, 0x11, 0xa, 0x2, 0x3, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 
       0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x5, 0x4, 0x20, 
       0xa, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x7, 0x5, 0x25, 0xa, 0x5, 
       0xc, 0x5, 0xe, 0x5, 0x28, 0xb, 0x5, 0x3, 0x6, 0x3, 0x6, 0x3, 0x7, 
       0x3, 0x7, 0x3, 0x7, 0x2, 0x2, 0x8, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 
       0x2, 0x2, 0x2, 0x2a, 0x2, 0x10, 0x3, 0x2, 0x2, 0x2, 0x4, 0x12, 0x3, 
       0x2, 0x2, 0x2, 0x6, 0x19, 0x3, 0x2, 0x2, 0x2, 0x8, 0x21, 0x3, 0x2, 
       0x2, 0x2, 0xa, 0x29, 0x3, 0x2, 0x2, 0x2, 0xc, 0x2b, 0x3, 0x2, 0x2, 
       0x2, 0xe, 0x11, 0x5, 0x4, 0x3, 0x2, 0xf, 0x11, 0x5, 0x6, 0x4, 0x2, 
       0x10, 0xe, 0x3, 0x2, 0x2, 0x2, 0x10, 0xf, 0x3, 0x2, 0x2, 0x2, 0x11, 
       0x3, 0x3, 0x2, 0x2, 0x2, 0x12, 0x13, 0x7, 0x3, 0x2, 0x2, 0x13, 0x14, 
       0x5, 0xa, 0x6, 0x2, 0x14, 0x15, 0x7, 0x4, 0x2, 0x2, 0x15, 0x16, 0x7, 
       0x8, 0x2, 0x2, 0x16, 0x17, 0x7, 0x5, 0x2, 0x2, 0x17, 0x18, 0x5, 0xc, 
       0x7, 0x2, 0x18, 0x5, 0x3, 0x2, 0x2, 0x2, 0x19, 0x1a, 0x7, 0x6, 0x2, 
       0x2, 0x1a, 0x1b, 0x5, 0x8, 0x5, 0x2, 0x1b, 0x1c, 0x7, 0x4, 0x2, 0x2, 
       0x1c, 0x1f, 0x7, 0x8, 0x2, 0x2, 0x1d, 0x1e, 0x7, 0x7, 0x2, 0x2, 0x1e, 
       0x20, 0x7, 0x8, 0x2, 0x2, 0x1f, 0x1d, 0x3, 0x2, 0x2, 0x2, 0x1f, 0x20, 
       0x3, 0x2, 0x2, 0x2, 0x20, 0x7, 0x3, 0x2, 0x2, 0x2, 0x21, 0x26, 0x7, 
       0x8, 0x2, 0x2, 0x22, 0x23, 0x7, 0x19, 0x2, 0x2, 0x23, 0x25, 0x7, 
       0x8, 0x2, 0x2, 0x24, 0x22, 0x3, 0x2, 0x2, 0x2, 0x25, 0x28, 0x3, 0x2, 
       0x2, 0x2, 0x26, 0x24, 0x3, 0x2, 0x2, 0x2, 0x26, 0x27, 0x3, 0x2, 0x2, 
       0x2, 0x27, 0x9, 0x3, 0x2, 0x2, 0x2, 0x28, 0x26, 0x3, 0x2, 0x2, 0x2, 
       0x29, 0x2a, 0x7, 0x8, 0x2, 0x2, 0x2a, 0xb, 0x3, 0x2, 0x2, 0x2, 0x2b, 
       0x2c, 0x7, 0x8, 0x2, 0x2, 0x2c, 0xd, 0x3, 0x2, 0x2, 0x2, 0x5, 0x10, 
       0x1f, 0x26, 
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
