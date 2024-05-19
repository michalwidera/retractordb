
// Generated from DESC.g4 by ANTLR 4.13.1

#include "DESCLexer.h"

using namespace antlr4;

using namespace antlr4;

namespace {

struct DESCLexerStaticData final {
  DESCLexerStaticData(std::vector<std::string> ruleNames, std::vector<std::string> channelNames,
                      std::vector<std::string> modeNames, std::vector<std::string> literalNames,
                      std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)),
        channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)),
        literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  DESCLexerStaticData(const DESCLexerStaticData &)            = delete;
  DESCLexerStaticData(DESCLexerStaticData &&)                 = delete;
  DESCLexerStaticData &operator=(const DESCLexerStaticData &) = delete;
  DESCLexerStaticData &operator=(DESCLexerStaticData &&)      = delete;

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
      std::vector<std::string>{"T__0",      "T__1",       "T__2",         "T__3",    "T__4",     "BYTE_T",
                               "STRING_T",  "UNSIGNED_T", "INTEGER_T",    "FLOAT_T", "DOUBLE_T", "RATIONAL_T",
                               "INTPAIR_T", "IDXPAIR_T",  "DEC_DIGIT",    "TYPE_T",  "REF_T",    "ID",
                               "STRING",    "DECIMAL",    "REF_TYPE_ARG", "SPACE"},
      std::vector<std::string>{"DEFAULT_TOKEN_CHANNEL", "HIDDEN"}, std::vector<std::string>{"DEFAULT_MODE"},
      std::vector<std::string>{"", "'{'", "'}'", "'['", "']'", "'\"'", "'BYTE'", "'STRING'", "'UINT'", "'INTEGER'", "'FLOAT'",
                               "'DOUBLE'", "'RATIONAL'", "'INTPAIR'", "'IDXPAIR'", "'TYPE'", "'REF'"},
      std::vector<std::string>{"",          "",           "",          "",        "",         "",           "BYTE_T",
                               "STRING_T",  "UNSIGNED_T", "INTEGER_T", "FLOAT_T", "DOUBLE_T", "RATIONAL_T", "INTPAIR_T",
                               "IDXPAIR_T", "TYPE_T",     "REF_T",     "ID",      "STRING",   "DECIMAL",    "REF_TYPE_ARG",
                               "SPACE"});
  static const int32_t serializedATNSegment[] = {
      4,   0,   21, 191, 6,   -1, 2,   0,   7,   0,   2,   1,   7,   1,   2,  2,  7,   2,  2,   3,   7,  3,   2,   4,
      7,   4,   2,  5,   7,   5,  2,   6,   7,   6,   2,   7,   7,   7,   2,  8,  7,   8,  2,   9,   7,  9,   2,   10,
      7,   10,  2,  11,  7,   11, 2,   12,  7,   12,  2,   13,  7,   13,  2,  14, 7,   14, 2,   15,  7,  15,  2,   16,
      7,   16,  2,  17,  7,   17, 2,   18,  7,   18,  2,   19,  7,   19,  2,  20, 7,   20, 2,   21,  7,  21,  1,   0,
      1,   0,   1,  1,   1,   1,  1,   2,   1,   2,   1,   3,   1,   3,   1,  4,  1,   4,  1,   5,   1,  5,   1,   5,
      1,   5,   1,  5,   1,   6,  1,   6,   1,   6,   1,   6,   1,   6,   1,  6,  1,   6,  1,   7,   1,  7,   1,   7,
      1,   7,   1,  7,   1,   8,  1,   8,   1,   8,   1,   8,   1,   8,   1,  8,  1,   8,  1,   8,   1,  9,   1,   9,
      1,   9,   1,  9,   1,   9,  1,   9,   1,   10,  1,   10,  1,   10,  1,  10, 1,   10, 1,   10,  1,  10,  1,   11,
      1,   11,  1,  11,  1,   11, 1,   11,  1,   11,  1,   11,  1,   11,  1,  11, 1,   12, 1,   12,  1,  12,  1,   12,
      1,   12,  1,  12,  1,   12, 1,   12,  1,   13,  1,   13,  1,   13,  1,  13, 1,   13, 1,   13,  1,  13,  1,   13,
      1,   14,  1,  14,  1,   15, 1,   15,  1,   15,  1,   15,  1,   15,  1,  16, 1,   16, 1,   16,  1,  16,  1,   17,
      1,   17,  5,  17,  132, 8,  17,  10,  17,  12,  17,  135, 9,   17,  1,  18, 1,   18, 1,   18,  1,  18,  5,   18,
      141, 8,   18, 10,  18,  12, 18,  144, 9,   18,  1,   18,  1,   18,  1,  19, 4,   19, 149, 8,   19, 11,  19,  12,
      19,  150, 1,  20,  1,   20, 1,   20,  1,   20,  1,   20,  1,   20,  1,  20, 1,   20, 1,   20,  1,  20,  1,   20,
      1,   20,  1,  20,  1,   20, 1,   20,  1,   20,  1,   20,  1,   20,  1,  20, 1,   20, 1,   20,  1,  20,  1,   20,
      1,   20,  1,  20,  1,   20, 1,   20,  1,   20,  1,   20,  1,   20,  3,  20, 183, 8,  20,  1,   21, 4,   21,  186,
      8,   21,  11, 21,  12,  21, 187, 1,   21,  1,   21,  0,   0,   22,  1,  1,  3,   2,  5,   3,   7,  4,   9,   5,
      11,  6,   13, 7,   15,  8,  17,  9,   19,  10,  21,  11,  23,  12,  25, 13, 27,  14, 29,  0,   31, 15,  33,  16,
      35,  17,  37, 18,  39,  19, 41,  20,  43,  21,  1,   0,   5,   1,   0,  48, 57,  2,  0,   65,  90, 97,  122, 5,
      0,   36,  36, 48,  57,  65, 90,  95,  95,  97,  122, 1,   0,   39,  39, 3,  0,   9,  10,  13,  13, 32,  32,  197,
      0,   1,   1,  0,   0,   0,  0,   3,   1,   0,   0,   0,   0,   5,   1,  0,  0,   0,  0,   7,   1,  0,   0,   0,
      0,   9,   1,  0,   0,   0,  0,   11,  1,   0,   0,   0,   0,   13,  1,  0,  0,   0,  0,   15,  1,  0,   0,   0,
      0,   17,  1,  0,   0,   0,  0,   19,  1,   0,   0,   0,   0,   21,  1,  0,  0,   0,  0,   23,  1,  0,   0,   0,
      0,   25,  1,  0,   0,   0,  0,   27,  1,   0,   0,   0,   0,   31,  1,  0,  0,   0,  0,   33,  1,  0,   0,   0,
      0,   35,  1,  0,   0,   0,  0,   37,  1,   0,   0,   0,   0,   39,  1,  0,  0,   0,  0,   41,  1,  0,   0,   0,
      0,   43,  1,  0,   0,   0,  1,   45,  1,   0,   0,   0,   3,   47,  1,  0,  0,   0,  5,   49,  1,  0,   0,   0,
      7,   51,  1,  0,   0,   0,  9,   53,  1,   0,   0,   0,   11,  55,  1,  0,  0,   0,  13,  60,  1,  0,   0,   0,
      15,  67,  1,  0,   0,   0,  17,  72,  1,   0,   0,   0,   19,  80,  1,  0,  0,   0,  21,  86,  1,  0,   0,   0,
      23,  93,  1,  0,   0,   0,  25,  102, 1,   0,   0,   0,   27,  110, 1,  0,  0,   0,  29,  118, 1,  0,   0,   0,
      31,  120, 1,  0,   0,   0,  33,  125, 1,   0,   0,   0,   35,  129, 1,  0,  0,   0,  37,  136, 1,  0,   0,   0,
      39,  148, 1,  0,   0,   0,  41,  182, 1,   0,   0,   0,   43,  185, 1,  0,  0,   0,  45,  46,  5,  123, 0,   0,
      46,  2,   1,  0,   0,   0,  47,  48,  5,   125, 0,   0,   48,  4,   1,  0,  0,   0,  49,  50,  5,  91,  0,   0,
      50,  6,   1,  0,   0,   0,  51,  52,  5,   93,  0,   0,   52,  8,   1,  0,  0,   0,  53,  54,  5,  34,  0,   0,
      54,  10,  1,  0,   0,   0,  55,  56,  5,   66,  0,   0,   56,  57,  5,  89, 0,   0,  57,  58,  5,  84,  0,   0,
      58,  59,  5,  69,  0,   0,  59,  12,  1,   0,   0,   0,   60,  61,  5,  83, 0,   0,  61,  62,  5,  84,  0,   0,
      62,  63,  5,  82,  0,   0,  63,  64,  5,   73,  0,   0,   64,  65,  5,  78, 0,   0,  65,  66,  5,  71,  0,   0,
      66,  14,  1,  0,   0,   0,  67,  68,  5,   85,  0,   0,   68,  69,  5,  73, 0,   0,  69,  70,  5,  78,  0,   0,
      70,  71,  5,  84,  0,   0,  71,  16,  1,   0,   0,   0,   72,  73,  5,  73, 0,   0,  73,  74,  5,  78,  0,   0,
      74,  75,  5,  84,  0,   0,  75,  76,  5,   69,  0,   0,   76,  77,  5,  71, 0,   0,  77,  78,  5,  69,  0,   0,
      78,  79,  5,  82,  0,   0,  79,  18,  1,   0,   0,   0,   80,  81,  5,  70, 0,   0,  81,  82,  5,  76,  0,   0,
      82,  83,  5,  79,  0,   0,  83,  84,  5,   65,  0,   0,   84,  85,  5,  84, 0,   0,  85,  20,  1,  0,   0,   0,
      86,  87,  5,  68,  0,   0,  87,  88,  5,   79,  0,   0,   88,  89,  5,  85, 0,   0,  89,  90,  5,  66,  0,   0,
      90,  91,  5,  76,  0,   0,  91,  92,  5,   69,  0,   0,   92,  22,  1,  0,  0,   0,  93,  94,  5,  82,  0,   0,
      94,  95,  5,  65,  0,   0,  95,  96,  5,   84,  0,   0,   96,  97,  5,  73, 0,   0,  97,  98,  5,  79,  0,   0,
      98,  99,  5,  78,  0,   0,  99,  100, 5,   65,  0,   0,   100, 101, 5,  76, 0,   0,  101, 24,  1,  0,   0,   0,
      102, 103, 5,  73,  0,   0,  103, 104, 5,   78,  0,   0,   104, 105, 5,  84, 0,   0,  105, 106, 5,  80,  0,   0,
      106, 107, 5,  65,  0,   0,  107, 108, 5,   73,  0,   0,   108, 109, 5,  82, 0,   0,  109, 26,  1,  0,   0,   0,
      110, 111, 5,  73,  0,   0,  111, 112, 5,   68,  0,   0,   112, 113, 5,  88, 0,   0,  113, 114, 5,  80,  0,   0,
      114, 115, 5,  65,  0,   0,  115, 116, 5,   73,  0,   0,   116, 117, 5,  82, 0,   0,  117, 28,  1,  0,   0,   0,
      118, 119, 7,  0,   0,   0,  119, 30,  1,   0,   0,   0,   120, 121, 5,  84, 0,   0,  121, 122, 5,  89,  0,   0,
      122, 123, 5,  80,  0,   0,  123, 124, 5,   69,  0,   0,   124, 32,  1,  0,  0,   0,  125, 126, 5,  82,  0,   0,
      126, 127, 5,  69,  0,   0,  127, 128, 5,   70,  0,   0,   128, 34,  1,  0,  0,   0,  129, 133, 7,  1,   0,   0,
      130, 132, 7,  2,   0,   0,  131, 130, 1,   0,   0,   0,   132, 135, 1,  0,  0,   0,  133, 131, 1,  0,   0,   0,
      133, 134, 1,  0,   0,   0,  134, 36,  1,   0,   0,   0,   135, 133, 1,  0,  0,   0,  136, 142, 5,  39,  0,   0,
      137, 141, 8,  3,   0,   0,  138, 139, 5,   39,  0,   0,   139, 141, 5,  39, 0,   0,  140, 137, 1,  0,   0,   0,
      140, 138, 1,  0,   0,   0,  141, 144, 1,   0,   0,   0,   142, 140, 1,  0,  0,   0,  142, 143, 1,  0,   0,   0,
      143, 145, 1,  0,   0,   0,  144, 142, 1,   0,   0,   0,   145, 146, 5,  39, 0,   0,  146, 38,  1,  0,   0,   0,
      147, 149, 3,  29,  14,  0,  148, 147, 1,   0,   0,   0,   149, 150, 1,  0,  0,   0,  150, 148, 1,  0,   0,   0,
      150, 151, 1,  0,   0,   0,  151, 40,  1,   0,   0,   0,   152, 153, 5,  84, 0,   0,  153, 154, 5,  69,  0,   0,
      154, 155, 5,  88,  0,   0,  155, 156, 5,   84,  0,   0,   156, 157, 5,  83, 0,   0,  157, 158, 5,  79,  0,   0,
      158, 159, 5,  85,  0,   0,  159, 160, 5,   82,  0,   0,   160, 161, 5,  67, 0,   0,  161, 183, 5,  69,  0,   0,
      162, 163, 5,  68,  0,   0,  163, 164, 5,   69,  0,   0,   164, 165, 5,  86, 0,   0,  165, 166, 5,  73,  0,   0,
      166, 167, 5,  67,  0,   0,  167, 183, 5,   69,  0,   0,   168, 169, 5,  71, 0,   0,  169, 170, 5,  69,  0,   0,
      170, 171, 5,  78,  0,   0,  171, 172, 5,   69,  0,   0,   172, 173, 5,  82, 0,   0,  173, 174, 5,  73,  0,   0,
      174, 183, 5,  67,  0,   0,  175, 176, 5,   68,  0,   0,   176, 177, 5,  69, 0,   0,  177, 178, 5,  70,  0,   0,
      178, 179, 5,  65,  0,   0,  179, 180, 5,   85,  0,   0,   180, 181, 5,  76, 0,   0,  181, 183, 5,  84,  0,   0,
      182, 152, 1,  0,   0,   0,  182, 162, 1,   0,   0,   0,   182, 168, 1,  0,  0,   0,  182, 175, 1,  0,   0,   0,
      183, 42,  1,  0,   0,   0,  184, 186, 7,   4,   0,   0,   185, 184, 1,  0,  0,   0,  186, 187, 1,  0,   0,   0,
      187, 185, 1,  0,   0,   0,  187, 188, 1,   0,   0,   0,   188, 189, 1,  0,  0,   0,  189, 190, 6,  21,  0,   0,
      190, 44,  1,  0,   0,   0,  7,   0,   133, 140, 142, 150, 182, 187, 1,  6,  0,   0};
  staticData->serializedATN =
      antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) {
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  desclexerLexerStaticData = staticData.release();
}

}  // namespace

DESCLexer::DESCLexer(CharStream *input) : Lexer(input) {
  DESCLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *desclexerLexerStaticData->atn, desclexerLexerStaticData->decisionToDFA,
                                            desclexerLexerStaticData->sharedContextCache);
}

DESCLexer::~DESCLexer() { delete _interpreter; }

std::string DESCLexer::getGrammarFileName() const { return "DESC.g4"; }

const std::vector<std::string> &DESCLexer::getRuleNames() const { return desclexerLexerStaticData->ruleNames; }

const std::vector<std::string> &DESCLexer::getChannelNames() const { return desclexerLexerStaticData->channelNames; }

const std::vector<std::string> &DESCLexer::getModeNames() const { return desclexerLexerStaticData->modeNames; }

const dfa::Vocabulary &DESCLexer::getVocabulary() const { return desclexerLexerStaticData->vocabulary; }

antlr4::atn::SerializedATNView DESCLexer::getSerializedATN() const { return desclexerLexerStaticData->serializedATN; }

const atn::ATN &DESCLexer::getATN() const { return *desclexerLexerStaticData->atn; }

void DESCLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  desclexerLexerInitialize();
#else
  ::antlr4::internal::call_once(desclexerLexerOnceFlag, desclexerLexerInitialize);
#endif
}
