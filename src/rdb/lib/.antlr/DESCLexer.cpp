
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
      std::vector<std::string>{"T__0",       "T__1",         "T__2",     "T__3",        "T__4",       "BYTE_T",    "STRING_T",
                               "UNSIGNED_T", "INTEGER_T",    "FLOAT_T",  "DOUBLE_T",    "RATIONAL_T", "INTPAIR_T", "IDXPAIR_T",
                               "DEC_DIGIT",  "TYPE_T",       "REF_T",    "RETENTION_T", "DOT",        "ID",        "STRING",
                               "DECIMAL",    "REF_TYPE_ARG", "FILENAME", "SPACE"},
      std::vector<std::string>{"DEFAULT_TOKEN_CHANNEL", "HIDDEN"}, std::vector<std::string>{"DEFAULT_MODE"},
      std::vector<std::string>{"", "'{'", "'}'", "'['", "']'", "'\"'", "'BYTE'", "'STRING'", "'UINT'", "'INTEGER'", "'FLOAT'",
                               "'DOUBLE'", "'RATIONAL'", "'INTPAIR'", "'IDXPAIR'", "'TYPE'", "'REF'", "'RETENTION'", "'.'"},
      std::vector<std::string>{"",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "BYTE_T",
                               "STRING_T",
                               "UNSIGNED_T",
                               "INTEGER_T",
                               "FLOAT_T",
                               "DOUBLE_T",
                               "RATIONAL_T",
                               "INTPAIR_T",
                               "IDXPAIR_T",
                               "TYPE_T",
                               "REF_T",
                               "RETENTION_T",
                               "DOT",
                               "ID",
                               "STRING",
                               "DECIMAL",
                               "REF_TYPE_ARG",
                               "FILENAME",
                               "SPACE"});
  static const int32_t serializedATNSegment[] = {
      4,  0,   24,  215, 6,   -1,  2,   0,   7,   0,   2,   1,   7,   1,   2,   2,   7,   2,   2,   3,   7,   3,   2,  4,
      7,  4,   2,   5,   7,   5,   2,   6,   7,   6,   2,   7,   7,   7,   2,   8,   7,   8,   2,   9,   7,   9,   2,  10,
      7,  10,  2,   11,  7,   11,  2,   12,  7,   12,  2,   13,  7,   13,  2,   14,  7,   14,  2,   15,  7,   15,  2,  16,
      7,  16,  2,   17,  7,   17,  2,   18,  7,   18,  2,   19,  7,   19,  2,   20,  7,   20,  2,   21,  7,   21,  2,  22,
      7,  22,  2,   23,  7,   23,  2,   24,  7,   24,  1,   0,   1,   0,   1,   1,   1,   1,   1,   2,   1,   2,   1,  3,
      1,  3,   1,   4,   1,   4,   1,   5,   1,   5,   1,   5,   1,   5,   1,   5,   1,   6,   1,   6,   1,   6,   1,  6,
      1,  6,   1,   6,   1,   6,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   8,   1,   8,   1,   8,   1,  8,
      1,  8,   1,   8,   1,   8,   1,   8,   1,   9,   1,   9,   1,   9,   1,   9,   1,   9,   1,   9,   1,   10,  1,  10,
      1,  10,  1,   10,  1,   10,  1,   10,  1,   10,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,  11,
      1,  11,  1,   11,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   13,  1,  13,
      1,  13,  1,   13,  1,   13,  1,   13,  1,   13,  1,   13,  1,   14,  1,   14,  1,   15,  1,   15,  1,   15,  1,  15,
      1,  15,  1,   16,  1,   16,  1,   16,  1,   16,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,  17,
      1,  17,  1,   17,  1,   17,  1,   18,  1,   18,  1,   19,  1,   19,  5,   19,  150, 8,   19,  10,  19,  12,  19, 153,
      9,  19,  1,   20,  1,   20,  1,   20,  1,   20,  5,   20,  159, 8,   20,  10,  20,  12,  20,  162, 9,   20,  1,  20,
      1,  20,  1,   21,  4,   21,  167, 8,   21,  11,  21,  12,  21,  168, 1,   22,  1,   22,  1,   22,  1,   22,  1,  22,
      1,  22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,  22,
      1,  22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,   22,  1,  22,
      1,  22,  3,   22,  201, 8,   22,  1,   23,  1,   23,  4,   23,  205, 8,   23,  11,  23,  12,  23,  206, 1,   24, 4,
      24, 210, 8,   24,  11,  24,  12,  24,  211, 1,   24,  1,   24,  0,   0,   25,  1,   1,   3,   2,   5,   3,   7,  4,
      9,  5,   11,  6,   13,  7,   15,  8,   17,  9,   19,  10,  21,  11,  23,  12,  25,  13,  27,  14,  29,  0,   31, 15,
      33, 16,  35,  17,  37,  18,  39,  19,  41,  20,  43,  21,  45,  22,  47,  23,  49,  24,  1,   0,   5,   1,   0,  48,
      57, 2,   0,   65,  90,  97,  122, 5,   0,   36,  36,  48,  57,  65,  90,  95,  95,  97,  122, 1,   0,   39,  39, 3,
      0,  9,   10,  13,  13,  32,  32,  223, 0,   1,   1,   0,   0,   0,   0,   3,   1,   0,   0,   0,   0,   5,   1,  0,
      0,  0,   0,   7,   1,   0,   0,   0,   0,   9,   1,   0,   0,   0,   0,   11,  1,   0,   0,   0,   0,   13,  1,  0,
      0,  0,   0,   15,  1,   0,   0,   0,   0,   17,  1,   0,   0,   0,   0,   19,  1,   0,   0,   0,   0,   21,  1,  0,
      0,  0,   0,   23,  1,   0,   0,   0,   0,   25,  1,   0,   0,   0,   0,   27,  1,   0,   0,   0,   0,   31,  1,  0,
      0,  0,   0,   33,  1,   0,   0,   0,   0,   35,  1,   0,   0,   0,   0,   37,  1,   0,   0,   0,   0,   39,  1,  0,
      0,  0,   0,   41,  1,   0,   0,   0,   0,   43,  1,   0,   0,   0,   0,   45,  1,   0,   0,   0,   0,   47,  1,  0,
      0,  0,   0,   49,  1,   0,   0,   0,   1,   51,  1,   0,   0,   0,   3,   53,  1,   0,   0,   0,   5,   55,  1,  0,
      0,  0,   7,   57,  1,   0,   0,   0,   9,   59,  1,   0,   0,   0,   11,  61,  1,   0,   0,   0,   13,  66,  1,  0,
      0,  0,   15,  73,  1,   0,   0,   0,   17,  78,  1,   0,   0,   0,   19,  86,  1,   0,   0,   0,   21,  92,  1,  0,
      0,  0,   23,  99,  1,   0,   0,   0,   25,  108, 1,   0,   0,   0,   27,  116, 1,   0,   0,   0,   29,  124, 1,  0,
      0,  0,   31,  126, 1,   0,   0,   0,   33,  131, 1,   0,   0,   0,   35,  135, 1,   0,   0,   0,   37,  145, 1,  0,
      0,  0,   39,  147, 1,   0,   0,   0,   41,  154, 1,   0,   0,   0,   43,  166, 1,   0,   0,   0,   45,  200, 1,  0,
      0,  0,   47,  204, 1,   0,   0,   0,   49,  209, 1,   0,   0,   0,   51,  52,  5,   123, 0,   0,   52,  2,   1,  0,
      0,  0,   53,  54,  5,   125, 0,   0,   54,  4,   1,   0,   0,   0,   55,  56,  5,   91,  0,   0,   56,  6,   1,  0,
      0,  0,   57,  58,  5,   93,  0,   0,   58,  8,   1,   0,   0,   0,   59,  60,  5,   34,  0,   0,   60,  10,  1,  0,
      0,  0,   61,  62,  5,   66,  0,   0,   62,  63,  5,   89,  0,   0,   63,  64,  5,   84,  0,   0,   64,  65,  5,  69,
      0,  0,   65,  12,  1,   0,   0,   0,   66,  67,  5,   83,  0,   0,   67,  68,  5,   84,  0,   0,   68,  69,  5,  82,
      0,  0,   69,  70,  5,   73,  0,   0,   70,  71,  5,   78,  0,   0,   71,  72,  5,   71,  0,   0,   72,  14,  1,  0,
      0,  0,   73,  74,  5,   85,  0,   0,   74,  75,  5,   73,  0,   0,   75,  76,  5,   78,  0,   0,   76,  77,  5,  84,
      0,  0,   77,  16,  1,   0,   0,   0,   78,  79,  5,   73,  0,   0,   79,  80,  5,   78,  0,   0,   80,  81,  5,  84,
      0,  0,   81,  82,  5,   69,  0,   0,   82,  83,  5,   71,  0,   0,   83,  84,  5,   69,  0,   0,   84,  85,  5,  82,
      0,  0,   85,  18,  1,   0,   0,   0,   86,  87,  5,   70,  0,   0,   87,  88,  5,   76,  0,   0,   88,  89,  5,  79,
      0,  0,   89,  90,  5,   65,  0,   0,   90,  91,  5,   84,  0,   0,   91,  20,  1,   0,   0,   0,   92,  93,  5,  68,
      0,  0,   93,  94,  5,   79,  0,   0,   94,  95,  5,   85,  0,   0,   95,  96,  5,   66,  0,   0,   96,  97,  5,  76,
      0,  0,   97,  98,  5,   69,  0,   0,   98,  22,  1,   0,   0,   0,   99,  100, 5,   82,  0,   0,   100, 101, 5,  65,
      0,  0,   101, 102, 5,   84,  0,   0,   102, 103, 5,   73,  0,   0,   103, 104, 5,   79,  0,   0,   104, 105, 5,  78,
      0,  0,   105, 106, 5,   65,  0,   0,   106, 107, 5,   76,  0,   0,   107, 24,  1,   0,   0,   0,   108, 109, 5,  73,
      0,  0,   109, 110, 5,   78,  0,   0,   110, 111, 5,   84,  0,   0,   111, 112, 5,   80,  0,   0,   112, 113, 5,  65,
      0,  0,   113, 114, 5,   73,  0,   0,   114, 115, 5,   82,  0,   0,   115, 26,  1,   0,   0,   0,   116, 117, 5,  73,
      0,  0,   117, 118, 5,   68,  0,   0,   118, 119, 5,   88,  0,   0,   119, 120, 5,   80,  0,   0,   120, 121, 5,  65,
      0,  0,   121, 122, 5,   73,  0,   0,   122, 123, 5,   82,  0,   0,   123, 28,  1,   0,   0,   0,   124, 125, 7,  0,
      0,  0,   125, 30,  1,   0,   0,   0,   126, 127, 5,   84,  0,   0,   127, 128, 5,   89,  0,   0,   128, 129, 5,  80,
      0,  0,   129, 130, 5,   69,  0,   0,   130, 32,  1,   0,   0,   0,   131, 132, 5,   82,  0,   0,   132, 133, 5,  69,
      0,  0,   133, 134, 5,   70,  0,   0,   134, 34,  1,   0,   0,   0,   135, 136, 5,   82,  0,   0,   136, 137, 5,  69,
      0,  0,   137, 138, 5,   84,  0,   0,   138, 139, 5,   69,  0,   0,   139, 140, 5,   78,  0,   0,   140, 141, 5,  84,
      0,  0,   141, 142, 5,   73,  0,   0,   142, 143, 5,   79,  0,   0,   143, 144, 5,   78,  0,   0,   144, 36,  1,  0,
      0,  0,   145, 146, 5,   46,  0,   0,   146, 38,  1,   0,   0,   0,   147, 151, 7,   1,   0,   0,   148, 150, 7,  2,
      0,  0,   149, 148, 1,   0,   0,   0,   150, 153, 1,   0,   0,   0,   151, 149, 1,   0,   0,   0,   151, 152, 1,  0,
      0,  0,   152, 40,  1,   0,   0,   0,   153, 151, 1,   0,   0,   0,   154, 160, 5,   39,  0,   0,   155, 159, 8,  3,
      0,  0,   156, 157, 5,   39,  0,   0,   157, 159, 5,   39,  0,   0,   158, 155, 1,   0,   0,   0,   158, 156, 1,  0,
      0,  0,   159, 162, 1,   0,   0,   0,   160, 158, 1,   0,   0,   0,   160, 161, 1,   0,   0,   0,   161, 163, 1,  0,
      0,  0,   162, 160, 1,   0,   0,   0,   163, 164, 5,   39,  0,   0,   164, 42,  1,   0,   0,   0,   165, 167, 3,  29,
      14, 0,   166, 165, 1,   0,   0,   0,   167, 168, 1,   0,   0,   0,   168, 166, 1,   0,   0,   0,   168, 169, 1,  0,
      0,  0,   169, 44,  1,   0,   0,   0,   170, 171, 5,   84,  0,   0,   171, 172, 5,   69,  0,   0,   172, 173, 5,  88,
      0,  0,   173, 174, 5,   84,  0,   0,   174, 175, 5,   83,  0,   0,   175, 176, 5,   79,  0,   0,   176, 177, 5,  85,
      0,  0,   177, 178, 5,   82,  0,   0,   178, 179, 5,   67,  0,   0,   179, 201, 5,   69,  0,   0,   180, 181, 5,  68,
      0,  0,   181, 182, 5,   69,  0,   0,   182, 183, 5,   86,  0,   0,   183, 184, 5,   73,  0,   0,   184, 185, 5,  67,
      0,  0,   185, 201, 5,   69,  0,   0,   186, 187, 5,   71,  0,   0,   187, 188, 5,   69,  0,   0,   188, 189, 5,  78,
      0,  0,   189, 190, 5,   69,  0,   0,   190, 191, 5,   82,  0,   0,   191, 192, 5,   73,  0,   0,   192, 201, 5,  67,
      0,  0,   193, 194, 5,   68,  0,   0,   194, 195, 5,   69,  0,   0,   195, 196, 5,   70,  0,   0,   196, 197, 5,  65,
      0,  0,   197, 198, 5,   85,  0,   0,   198, 199, 5,   76,  0,   0,   199, 201, 5,   84,  0,   0,   200, 170, 1,  0,
      0,  0,   200, 180, 1,   0,   0,   0,   200, 186, 1,   0,   0,   0,   200, 193, 1,   0,   0,   0,   201, 46,  1,  0,
      0,  0,   202, 205, 7,   2,   0,   0,   203, 205, 3,   37,  18,  0,   204, 202, 1,   0,   0,   0,   204, 203, 1,  0,
      0,  0,   205, 206, 1,   0,   0,   0,   206, 204, 1,   0,   0,   0,   206, 207, 1,   0,   0,   0,   207, 48,  1,  0,
      0,  0,   208, 210, 7,   4,   0,   0,   209, 208, 1,   0,   0,   0,   210, 211, 1,   0,   0,   0,   211, 209, 1,  0,
      0,  0,   211, 212, 1,   0,   0,   0,   212, 213, 1,   0,   0,   0,   213, 214, 6,   24,  0,   0,   214, 50,  1,  0,
      0,  0,   9,   0,   151, 158, 160, 168, 200, 204, 206, 211, 1,   6,   0,   0};
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
