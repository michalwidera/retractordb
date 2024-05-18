
// Generated from DESC.g4 by ANTLR 4.13.1


#include "DESCListener.h"

#include "DESCParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct DESCParserStaticData final {
  DESCParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  DESCParserStaticData(const DESCParserStaticData&) = delete;
  DESCParserStaticData(DESCParserStaticData&&) = delete;
  DESCParserStaticData& operator=(const DESCParserStaticData&) = delete;
  DESCParserStaticData& operator=(DESCParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag descParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
DESCParserStaticData *descParserStaticData = nullptr;

void descParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (descParserStaticData != nullptr) {
    return;
  }
#else
  assert(descParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<DESCParserStaticData>(
    std::vector<std::string>{
      "desc", "command"
    },
    std::vector<std::string>{
      "", "'{'", "'}'", "'BYTE'", "'STRING'", "'UINT'", "'INTEGER'", "'FLOAT'", 
      "'DOUBLE'", "'RATIONAl'", "'INTPAIR'", "'IDXPAIR'", "'TYPE'", "'REF'"
    },
    std::vector<std::string>{
      "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", "FLOAT_T", 
      "DOUBLE_T", "RATIONAL_T", "INTPAIR_T", "IDXPAIR_T", "TYPE_T", "REF_T", 
      "ID", "STRING", "REF_TYPE_ARG"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,16,30,2,0,7,0,2,1,7,1,1,0,1,0,5,0,7,8,0,10,0,12,0,10,9,0,1,0,1,0,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,28,8,1,1,
  	1,0,0,2,0,2,0,0,34,0,4,1,0,0,0,2,27,1,0,0,0,4,8,5,1,0,0,5,7,3,2,1,0,6,
  	5,1,0,0,0,7,10,1,0,0,0,8,6,1,0,0,0,8,9,1,0,0,0,9,11,1,0,0,0,10,8,1,0,
  	0,0,11,12,5,2,0,0,12,1,1,0,0,0,13,14,5,3,0,0,14,28,5,14,0,0,15,16,5,4,
  	0,0,16,28,5,14,0,0,17,18,5,5,0,0,18,28,5,14,0,0,19,20,5,7,0,0,20,28,5,
  	14,0,0,21,22,5,8,0,0,22,28,5,14,0,0,23,24,5,13,0,0,24,28,5,15,0,0,25,
  	26,5,12,0,0,26,28,5,16,0,0,27,13,1,0,0,0,27,15,1,0,0,0,27,17,1,0,0,0,
  	27,19,1,0,0,0,27,21,1,0,0,0,27,23,1,0,0,0,27,25,1,0,0,0,28,3,1,0,0,0,
  	2,8,27
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  descParserStaticData = staticData.release();
}

}

DESCParser::DESCParser(TokenStream *input) : DESCParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

DESCParser::DESCParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  DESCParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *descParserStaticData->atn, descParserStaticData->decisionToDFA, descParserStaticData->sharedContextCache, options);
}

DESCParser::~DESCParser() {
  delete _interpreter;
}

const atn::ATN& DESCParser::getATN() const {
  return *descParserStaticData->atn;
}

std::string DESCParser::getGrammarFileName() const {
  return "DESC.g4";
}

const std::vector<std::string>& DESCParser::getRuleNames() const {
  return descParserStaticData->ruleNames;
}

const dfa::Vocabulary& DESCParser::getVocabulary() const {
  return descParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView DESCParser::getSerializedATN() const {
  return descParserStaticData->serializedATN;
}


//----------------- DescContext ------------------------------------------------------------------

DESCParser::DescContext::DescContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<DESCParser::CommandContext *> DESCParser::DescContext::command() {
  return getRuleContexts<DESCParser::CommandContext>();
}

DESCParser::CommandContext* DESCParser::DescContext::command(size_t i) {
  return getRuleContext<DESCParser::CommandContext>(i);
}


size_t DESCParser::DescContext::getRuleIndex() const {
  return DESCParser::RuleDesc;
}

void DESCParser::DescContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDesc(this);
}

void DESCParser::DescContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDesc(this);
}

DESCParser::DescContext* DESCParser::desc() {
  DescContext *_localctx = _tracker.createInstance<DescContext>(_ctx, getState());
  enterRule(_localctx, 0, DESCParser::RuleDesc);
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
    setState(4);
    match(DESCParser::T__0);
    setState(8);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 12728) != 0)) {
      setState(5);
      command();
      setState(10);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(11);
    match(DESCParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CommandContext ------------------------------------------------------------------

DESCParser::CommandContext::CommandContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* DESCParser::CommandContext::BYTE_T() {
  return getToken(DESCParser::BYTE_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::CommandContext::STRING_T() {
  return getToken(DESCParser::STRING_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::UNSIGNED_T() {
  return getToken(DESCParser::UNSIGNED_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::FLOAT_T() {
  return getToken(DESCParser::FLOAT_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::DOUBLE_T() {
  return getToken(DESCParser::DOUBLE_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::REF_T() {
  return getToken(DESCParser::REF_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::STRING() {
  return getToken(DESCParser::STRING, 0);
}

tree::TerminalNode* DESCParser::CommandContext::TYPE_T() {
  return getToken(DESCParser::TYPE_T, 0);
}

tree::TerminalNode* DESCParser::CommandContext::REF_TYPE_ARG() {
  return getToken(DESCParser::REF_TYPE_ARG, 0);
}


size_t DESCParser::CommandContext::getRuleIndex() const {
  return DESCParser::RuleCommand;
}

void DESCParser::CommandContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCommand(this);
}

void DESCParser::CommandContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCommand(this);
}

DESCParser::CommandContext* DESCParser::command() {
  CommandContext *_localctx = _tracker.createInstance<CommandContext>(_ctx, getState());
  enterRule(_localctx, 2, DESCParser::RuleCommand);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(27);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case DESCParser::BYTE_T: {
        enterOuterAlt(_localctx, 1);
        setState(13);
        match(DESCParser::BYTE_T);
        setState(14);
        antlrcpp::downCast<CommandContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::STRING_T: {
        enterOuterAlt(_localctx, 2);
        setState(15);
        match(DESCParser::STRING_T);
        setState(16);
        antlrcpp::downCast<CommandContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::UNSIGNED_T: {
        enterOuterAlt(_localctx, 3);
        setState(17);
        match(DESCParser::UNSIGNED_T);
        setState(18);
        antlrcpp::downCast<CommandContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::FLOAT_T: {
        enterOuterAlt(_localctx, 4);
        setState(19);
        match(DESCParser::FLOAT_T);
        setState(20);
        antlrcpp::downCast<CommandContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::DOUBLE_T: {
        enterOuterAlt(_localctx, 5);
        setState(21);
        match(DESCParser::DOUBLE_T);
        setState(22);
        antlrcpp::downCast<CommandContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::REF_T: {
        enterOuterAlt(_localctx, 6);
        setState(23);
        match(DESCParser::REF_T);
        setState(24);
        antlrcpp::downCast<CommandContext *>(_localctx)->name = match(DESCParser::STRING);
        break;
      }

      case DESCParser::TYPE_T: {
        enterOuterAlt(_localctx, 7);
        setState(25);
        match(DESCParser::TYPE_T);
        setState(26);
        match(DESCParser::REF_TYPE_ARG);
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

void DESCParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  descParserInitialize();
#else
  ::antlr4::internal::call_once(descParserOnceFlag, descParserInitialize);
#endif
}
