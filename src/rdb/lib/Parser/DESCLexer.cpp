
// Generated from DESC.g4 by ANTLR 4.13.1


#include "DESCLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct DESCLexerStaticData final {
  DESCLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  DESCLexerStaticData(const DESCLexerStaticData&) = delete;
  DESCLexerStaticData(DESCLexerStaticData&&) = delete;
  DESCLexerStaticData& operator=(const DESCLexerStaticData&) = delete;
  DESCLexerStaticData& operator=(DESCLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag desclexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
DESCLexerStaticData *desclexerLexerStaticData = nullptr;

void desclexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (desclexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(desclexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<DESCLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", "FLOAT_T", 
      "DOUBLE_T", "RATIONAL_T", "INTPAIR_T", "IDXPAIR_T", "TYPE_T", "REF_T", 
      "ID", "STRING", "REF_TYPE_ARG"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
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
  	4,0,16,159,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,1,0,1,0,1,1,1,1,1,2,1,2,1,2,1,2,1,2,1,3,1,3,1,3,1,3,1,
  	3,1,3,1,3,1,4,1,4,1,4,1,4,1,4,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,6,1,6,
  	1,6,1,6,1,6,1,6,1,7,1,7,1,7,1,7,1,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,
  	8,1,8,1,8,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,10,1,10,1,10,1,10,1,10,1,
  	10,1,10,1,10,1,11,1,11,1,11,1,11,1,11,1,12,1,12,1,12,1,12,1,13,1,13,5,
  	13,112,8,13,10,13,12,13,115,9,13,1,14,1,14,1,14,1,14,5,14,121,8,14,10,
  	14,12,14,124,9,14,1,14,1,14,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,
  	15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,
  	15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,3,15,158,8,15,0,0,16,1,1,3,2,5,
  	3,7,4,9,5,11,6,13,7,15,8,17,9,19,10,21,11,23,12,25,13,27,14,29,15,31,
  	16,1,0,3,2,0,65,90,97,122,5,0,36,36,48,57,65,90,95,95,97,122,1,0,39,39,
  	164,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,
  	1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,1,0,0,0,0,21,1,0,
  	0,0,0,23,1,0,0,0,0,25,1,0,0,0,0,27,1,0,0,0,0,29,1,0,0,0,0,31,1,0,0,0,
  	1,33,1,0,0,0,3,35,1,0,0,0,5,37,1,0,0,0,7,42,1,0,0,0,9,49,1,0,0,0,11,54,
  	1,0,0,0,13,62,1,0,0,0,15,68,1,0,0,0,17,75,1,0,0,0,19,84,1,0,0,0,21,92,
  	1,0,0,0,23,100,1,0,0,0,25,105,1,0,0,0,27,109,1,0,0,0,29,116,1,0,0,0,31,
  	157,1,0,0,0,33,34,5,123,0,0,34,2,1,0,0,0,35,36,5,125,0,0,36,4,1,0,0,0,
  	37,38,5,66,0,0,38,39,5,89,0,0,39,40,5,84,0,0,40,41,5,69,0,0,41,6,1,0,
  	0,0,42,43,5,83,0,0,43,44,5,84,0,0,44,45,5,82,0,0,45,46,5,73,0,0,46,47,
  	5,78,0,0,47,48,5,71,0,0,48,8,1,0,0,0,49,50,5,85,0,0,50,51,5,73,0,0,51,
  	52,5,78,0,0,52,53,5,84,0,0,53,10,1,0,0,0,54,55,5,73,0,0,55,56,5,78,0,
  	0,56,57,5,84,0,0,57,58,5,69,0,0,58,59,5,71,0,0,59,60,5,69,0,0,60,61,5,
  	82,0,0,61,12,1,0,0,0,62,63,5,70,0,0,63,64,5,76,0,0,64,65,5,79,0,0,65,
  	66,5,65,0,0,66,67,5,84,0,0,67,14,1,0,0,0,68,69,5,68,0,0,69,70,5,79,0,
  	0,70,71,5,85,0,0,71,72,5,66,0,0,72,73,5,76,0,0,73,74,5,69,0,0,74,16,1,
  	0,0,0,75,76,5,82,0,0,76,77,5,65,0,0,77,78,5,84,0,0,78,79,5,73,0,0,79,
  	80,5,79,0,0,80,81,5,78,0,0,81,82,5,65,0,0,82,83,5,108,0,0,83,18,1,0,0,
  	0,84,85,5,73,0,0,85,86,5,78,0,0,86,87,5,84,0,0,87,88,5,80,0,0,88,89,5,
  	65,0,0,89,90,5,73,0,0,90,91,5,82,0,0,91,20,1,0,0,0,92,93,5,73,0,0,93,
  	94,5,68,0,0,94,95,5,88,0,0,95,96,5,80,0,0,96,97,5,65,0,0,97,98,5,73,0,
  	0,98,99,5,82,0,0,99,22,1,0,0,0,100,101,5,84,0,0,101,102,5,89,0,0,102,
  	103,5,80,0,0,103,104,5,69,0,0,104,24,1,0,0,0,105,106,5,82,0,0,106,107,
  	5,69,0,0,107,108,5,70,0,0,108,26,1,0,0,0,109,113,7,0,0,0,110,112,7,1,
  	0,0,111,110,1,0,0,0,112,115,1,0,0,0,113,111,1,0,0,0,113,114,1,0,0,0,114,
  	28,1,0,0,0,115,113,1,0,0,0,116,122,5,39,0,0,117,121,8,2,0,0,118,119,5,
  	39,0,0,119,121,5,39,0,0,120,117,1,0,0,0,120,118,1,0,0,0,121,124,1,0,0,
  	0,122,120,1,0,0,0,122,123,1,0,0,0,123,125,1,0,0,0,124,122,1,0,0,0,125,
  	126,5,39,0,0,126,30,1,0,0,0,127,128,5,84,0,0,128,129,5,69,0,0,129,130,
  	5,88,0,0,130,131,5,84,0,0,131,132,5,83,0,0,132,133,5,79,0,0,133,134,5,
  	85,0,0,134,135,5,82,0,0,135,136,5,67,0,0,136,158,5,69,0,0,137,138,5,68,
  	0,0,138,139,5,69,0,0,139,140,5,86,0,0,140,141,5,73,0,0,141,142,5,67,0,
  	0,142,158,5,69,0,0,143,144,5,71,0,0,144,145,5,69,0,0,145,146,5,78,0,0,
  	146,147,5,69,0,0,147,148,5,82,0,0,148,149,5,73,0,0,149,158,5,67,0,0,150,
  	151,5,68,0,0,151,152,5,69,0,0,152,153,5,70,0,0,153,154,5,65,0,0,154,155,
  	5,85,0,0,155,156,5,76,0,0,156,158,5,84,0,0,157,127,1,0,0,0,157,137,1,
  	0,0,0,157,143,1,0,0,0,157,150,1,0,0,0,158,32,1,0,0,0,5,0,113,120,122,
  	157,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  desclexerLexerStaticData = staticData.release();
}

}

DESCLexer::DESCLexer(CharStream *input) : Lexer(input) {
  DESCLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *desclexerLexerStaticData->atn, desclexerLexerStaticData->decisionToDFA, desclexerLexerStaticData->sharedContextCache);
}

DESCLexer::~DESCLexer() {
  delete _interpreter;
}

std::string DESCLexer::getGrammarFileName() const {
  return "DESC.g4";
}

const std::vector<std::string>& DESCLexer::getRuleNames() const {
  return desclexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& DESCLexer::getChannelNames() const {
  return desclexerLexerStaticData->channelNames;
}

const std::vector<std::string>& DESCLexer::getModeNames() const {
  return desclexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& DESCLexer::getVocabulary() const {
  return desclexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView DESCLexer::getSerializedATN() const {
  return desclexerLexerStaticData->serializedATN;
}

const atn::ATN& DESCLexer::getATN() const {
  return *desclexerLexerStaticData->atn;
}




void DESCLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  desclexerLexerInitialize();
#else
  ::antlr4::internal::call_once(desclexerLexerOnceFlag, desclexerLexerInitialize);
#endif
}
