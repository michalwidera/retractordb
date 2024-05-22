
// Generated from RQL.g4 by ANTLR 4.13.1

#include "RQLLexer.h"

using namespace antlr4;

using namespace antlr4;

namespace {

struct RQLLexerStaticData final {
  RQLLexerStaticData(std::vector<std::string> ruleNames, std::vector<std::string> channelNames,
                     std::vector<std::string> modeNames, std::vector<std::string> literalNames,
                     std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)),
        channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)),
        literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RQLLexerStaticData(const RQLLexerStaticData &)            = delete;
  RQLLexerStaticData(RQLLexerStaticData &&)                 = delete;
  RQLLexerStaticData &operator=(const RQLLexerStaticData &) = delete;
  RQLLexerStaticData &operator=(RQLLexerStaticData &&)      = delete;

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

::antlr4::internal::OnceFlag rqllexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
    RQLLexerStaticData *rqllexerLexerStaticData = nullptr;

void rqllexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (rqllexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(rqllexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<RQLLexerStaticData>(
      std::vector<std::string>{
          "T__0",      "T__1",         "T__2",    "T__3",          "T__4",          "T__5",        "T__6",        "T__7",
          "T__8",      "T__9",         "T__10",   "T__11",         "T__12",         "T__13",       "T__14",       "T__15",
          "T__16",     "T__17",        "T__18",   "T__19",         "BYTE_T",        "STRING_T",    "UNSIGNED_T",  "INTEGER_T",
          "FLOAT_T",   "DOUBLE_T",     "SELECT",  "STREAM",        "FROM",          "DECLARE",     "RETENTION",   "FILE",
          "STORAGE",   "MIN",          "MAX",     "AVG",           "SUMC",          "ID",          "STRING",      "FLOAT",
          "DECIMAL",   "REAL",         "EQUAL",   "GREATER",       "LESS",          "EXCLAMATION", "DOUBLE_BAR",  "DOT",
          "UNDERLINE", "AT",           "SHARP",   "AND",           "MOD",           "DOLLAR",      "COMMA",       "SEMI",
          "COLON",     "DOUBLE_COLON", "STAR",    "DIVIDE",        "PLUS",          "MINUS",       "BIT_NOT",     "BIT_OR",
          "BIT_XOR",   "SPACE",        "COMMENT", "LINE_COMMENT1", "LINE_COMMENT2", "LETTER",      "DEC_DOT_DEC", "HEX_DIGIT",
          "DEC_DIGIT"},
      std::vector<std::string>{"DEFAULT_TOKEN_CHANNEL", "HIDDEN"}, std::vector<std::string>{"DEFAULT_MODE"},
      std::vector<std::string>{"",
                               "'['",
                               "']'",
                               "'('",
                               "')'",
                               "'Sqrt'",
                               "'Ceil'",
                               "'Abs'",
                               "'Floor'",
                               "'Sign'",
                               "'Chr'",
                               "'Length'",
                               "'ToNumber'",
                               "'ToTimeStamp'",
                               "'FloatCast'",
                               "'IntCast'",
                               "'Count'",
                               "'Crc'",
                               "'Sum'",
                               "'IsZero'",
                               "'IsNonZero'",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "'='",
                               "'>'",
                               "'<'",
                               "'!'",
                               "'||'",
                               "'.'",
                               "'_'",
                               "'@'",
                               "'#'",
                               "'&'",
                               "'%'",
                               "'$'",
                               "','",
                               "';'",
                               "':'",
                               "'::'",
                               "'*'",
                               "'/'",
                               "'+'",
                               "'-'",
                               "'~'",
                               "'|'",
                               "'^'"},
      std::vector<std::string>{"",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
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
                               "SELECT",
                               "STREAM",
                               "FROM",
                               "DECLARE",
                               "RETENTION",
                               "FILE",
                               "STORAGE",
                               "MIN",
                               "MAX",
                               "AVG",
                               "SUMC",
                               "ID",
                               "STRING",
                               "FLOAT",
                               "DECIMAL",
                               "REAL",
                               "EQUAL",
                               "GREATER",
                               "LESS",
                               "EXCLAMATION",
                               "DOUBLE_BAR",
                               "DOT",
                               "UNDERLINE",
                               "AT",
                               "SHARP",
                               "AND",
                               "MOD",
                               "DOLLAR",
                               "COMMA",
                               "SEMI",
                               "COLON",
                               "DOUBLE_COLON",
                               "STAR",
                               "DIVIDE",
                               "PLUS",
                               "MINUS",
                               "BIT_NOT",
                               "BIT_OR",
                               "BIT_XOR",
                               "SPACE",
                               "COMMENT",
                               "LINE_COMMENT1",
                               "LINE_COMMENT2"});
  static const int32_t serializedATNSegment[] = {
      4,   0,   69,  641, 6,   -1,  2,   0,   7,   0,   2,   1,   7,   1,   2,   2,   7,   2,   2,   3,   7,   3,   2,   4,
      7,   4,   2,   5,   7,   5,   2,   6,   7,   6,   2,   7,   7,   7,   2,   8,   7,   8,   2,   9,   7,   9,   2,   10,
      7,   10,  2,   11,  7,   11,  2,   12,  7,   12,  2,   13,  7,   13,  2,   14,  7,   14,  2,   15,  7,   15,  2,   16,
      7,   16,  2,   17,  7,   17,  2,   18,  7,   18,  2,   19,  7,   19,  2,   20,  7,   20,  2,   21,  7,   21,  2,   22,
      7,   22,  2,   23,  7,   23,  2,   24,  7,   24,  2,   25,  7,   25,  2,   26,  7,   26,  2,   27,  7,   27,  2,   28,
      7,   28,  2,   29,  7,   29,  2,   30,  7,   30,  2,   31,  7,   31,  2,   32,  7,   32,  2,   33,  7,   33,  2,   34,
      7,   34,  2,   35,  7,   35,  2,   36,  7,   36,  2,   37,  7,   37,  2,   38,  7,   38,  2,   39,  7,   39,  2,   40,
      7,   40,  2,   41,  7,   41,  2,   42,  7,   42,  2,   43,  7,   43,  2,   44,  7,   44,  2,   45,  7,   45,  2,   46,
      7,   46,  2,   47,  7,   47,  2,   48,  7,   48,  2,   49,  7,   49,  2,   50,  7,   50,  2,   51,  7,   51,  2,   52,
      7,   52,  2,   53,  7,   53,  2,   54,  7,   54,  2,   55,  7,   55,  2,   56,  7,   56,  2,   57,  7,   57,  2,   58,
      7,   58,  2,   59,  7,   59,  2,   60,  7,   60,  2,   61,  7,   61,  2,   62,  7,   62,  2,   63,  7,   63,  2,   64,
      7,   64,  2,   65,  7,   65,  2,   66,  7,   66,  2,   67,  7,   67,  2,   68,  7,   68,  2,   69,  7,   69,  2,   70,
      7,   70,  2,   71,  7,   71,  2,   72,  7,   72,  1,   0,   1,   0,   1,   1,   1,   1,   1,   2,   1,   2,   1,   3,
      1,   3,   1,   4,   1,   4,   1,   4,   1,   4,   1,   4,   1,   5,   1,   5,   1,   5,   1,   5,   1,   5,   1,   6,
      1,   6,   1,   6,   1,   6,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   8,   1,   8,   1,   8,
      1,   8,   1,   8,   1,   9,   1,   9,   1,   9,   1,   9,   1,   10,  1,   10,  1,   10,  1,   10,  1,   10,  1,   10,
      1,   10,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,   11,  1,   12,  1,   12,
      1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   12,  1,   13,  1,   13,
      1,   13,  1,   13,  1,   13,  1,   13,  1,   13,  1,   13,  1,   13,  1,   13,  1,   14,  1,   14,  1,   14,  1,   14,
      1,   14,  1,   14,  1,   14,  1,   14,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  1,   16,  1,   16,
      1,   16,  1,   16,  1,   17,  1,   17,  1,   17,  1,   17,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,
      1,   18,  1,   19,  1,   19,  1,   19,  1,   19,  1,   19,  1,   19,  1,   19,  1,   19,  1,   19,  1,   19,  1,   20,
      1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,  1,   20,
      1,   20,  1,   20,  1,   20,  3,   20,  278, 8,   20,  1,   21,  1,   21,  1,   21,  1,   21,  1,   21,  1,   21,  1,
      21,  1,   21,  1,   21,  1,   21,  1,   21,  1,   21,  3,   21,  292, 8,   21,  1,   22,  1,   22,  1,   22,  1,   22,
      1,   22,  1,   22,  1,   22,  1,   22,  3,   22,  302, 8,   22,  1,   23,  1,   23,  1,   23,  1,   23,  1,   23,  1,
      23,  1,   23,  1,   23,  1,   23,  1,   23,  1,   23,  1,   23,  1,   23,  1,   23,  3,   23,  318, 8,   23,  1,   24,
      1,   24,  1,   24,  1,   24,  1,   24,  1,   24,  1,   24,  1,   24,  1,   24,  1,   24,  3,   24,  330, 8,   24,  1,
      25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  1,   25,  3,
      25,  344, 8,   25,  1,   26,  1,   26,  1,   26,  1,   26,  1,   26,  1,   26,  1,   26,  1,   26,  1,   26,  1,   26,
      1,   26,  1,   26,  3,   26,  358, 8,   26,  1,   27,  1,   27,  1,   27,  1,   27,  1,   27,  1,   27,  1,   27,  1,
      27,  1,   27,  1,   27,  1,   27,  1,   27,  3,   27,  372, 8,   27,  1,   28,  1,   28,  1,   28,  1,   28,  1,   28,
      1,   28,  1,   28,  1,   28,  3,   28,  382, 8,   28,  1,   29,  1,   29,  1,   29,  1,   29,  1,   29,  1,   29,  1,
      29,  1,   29,  1,   29,  1,   29,  1,   29,  1,   29,  1,   29,  1,   29,  3,   29,  398, 8,   29,  1,   30,  1,   30,
      1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,  1,   30,
      1,   30,  1,   30,  1,   30,  1,   30,  3,   30,  418, 8,   30,  1,   31,  1,   31,  1,   31,  1,   31,  1,   31,  1,
      31,  1,   31,  1,   31,  3,   31,  428, 8,   31,  1,   32,  1,   32,  1,   32,  1,   32,  1,   32,  1,   32,  1,   32,
      1,   32,  1,   32,  1,   32,  1,   32,  1,   32,  1,   32,  1,   32,  3,   32,  444, 8,   32,  1,   33,  1,   33,  1,
      33,  1,   33,  1,   33,  1,   33,  3,   33,  452, 8,   33,  1,   34,  1,   34,  1,   34,  1,   34,  1,   34,  1,   34,
      3,   34,  460, 8,   34,  1,   35,  1,   35,  1,   35,  1,   35,  1,   35,  1,   35,  3,   35,  468, 8,   35,  1,   36,
      1,   36,  1,   36,  1,   36,  1,   36,  1,   36,  1,   36,  1,   36,  3,   36,  478, 8,   36,  1,   37,  1,   37,  5,
      37,  482, 8,   37,  10,  37,  12,  37,  485, 9,   37,  1,   38,  1,   38,  1,   38,  1,   38,  5,   38,  491, 8,   38,
      10,  38,  12,  38,  494, 9,   38,  1,   38,  1,   38,  1,   39,  1,   39,  1,   40,  4,   40,  501, 8,   40,  11,  40,
      12,  40,  502, 1,   41,  1,   41,  3,   41,  507, 8,   41,  1,   41,  1,   41,  3,   41,  511, 8,   41,  1,   41,  4,
      41,  514, 8,   41,  11,  41,  12,  41,  515, 1,   42,  1,   42,  1,   43,  1,   43,  1,   44,  1,   44,  1,   45,  1,
      45,  1,   46,  1,   46,  1,   46,  1,   47,  1,   47,  1,   48,  1,   48,  1,   49,  1,   49,  1,   50,  1,   50,  1,
      51,  1,   51,  1,   52,  1,   52,  1,   53,  1,   53,  1,   54,  1,   54,  1,   55,  1,   55,  1,   56,  1,   56,  1,
      57,  1,   57,  1,   57,  1,   58,  1,   58,  1,   59,  1,   59,  1,   60,  1,   60,  1,   61,  1,   61,  1,   62,  1,
      62,  1,   63,  1,   63,  1,   64,  1,   64,  1,   65,  4,   65,  567, 8,   65,  11,  65,  12,  65,  568, 1,   65,  1,
      65,  1,   66,  1,   66,  1,   66,  1,   66,  1,   66,  5,   66,  578, 8,   66,  10,  66,  12,  66,  581, 9,   66,  1,
      66,  1,   66,  1,   66,  1,   66,  1,   66,  1,   67,  1,   67,  1,   67,  1,   67,  5,   67,  592, 8,   67,  10,  67,
      12,  67,  595, 9,   67,  1,   67,  1,   67,  1,   68,  1,   68,  1,   68,  1,   68,  5,   68,  603, 8,   68,  10,  68,
      12,  68,  606, 9,   68,  1,   68,  1,   68,  1,   69,  1,   69,  1,   70,  4,   70,  613, 8,   70,  11,  70,  12,  70,
      614, 1,   70,  1,   70,  4,   70,  619, 8,   70,  11,  70,  12,  70,  620, 1,   70,  4,   70,  624, 8,   70,  11,  70,
      12,  70,  625, 1,   70,  1,   70,  1,   70,  1,   70,  4,   70,  632, 8,   70,  11,  70,  12,  70,  633, 3,   70,  636,
      8,   70,  1,   71,  1,   71,  1,   72,  1,   72,  1,   579, 0,   73,  1,   1,   3,   2,   5,   3,   7,   4,   9,   5,
      11,  6,   13,  7,   15,  8,   17,  9,   19,  10,  21,  11,  23,  12,  25,  13,  27,  14,  29,  15,  31,  16,  33,  17,
      35,  18,  37,  19,  39,  20,  41,  21,  43,  22,  45,  23,  47,  24,  49,  25,  51,  26,  53,  27,  55,  28,  57,  29,
      59,  30,  61,  31,  63,  32,  65,  33,  67,  34,  69,  35,  71,  36,  73,  37,  75,  38,  77,  39,  79,  40,  81,  41,
      83,  42,  85,  43,  87,  44,  89,  45,  91,  46,  93,  47,  95,  48,  97,  49,  99,  50,  101, 51,  103, 52,  105, 53,
      107, 54,  109, 55,  111, 56,  113, 57,  115, 58,  117, 59,  119, 60,  121, 61,  123, 62,  125, 63,  127, 64,  129, 65,
      131, 66,  133, 67,  135, 68,  137, 69,  139, 0,   141, 0,   143, 0,   145, 0,   1,   0,   9,   2,   0,   65,  90,  97,
      122, 5,   0,   36,  36,  48,  57,  65,  90,  95,  95,  97,  122, 1,   0,   39,  39,  2,   0,   43,  43,  45,  45,  3,
      0,   9,   10,  13,  13,  32,  32,  2,   0,   10,  10,  13,  13,  2,   0,   65,  90,  95,  95,  2,   0,   48,  57,  65,
      70,  1,   0,   48,  57,  673, 0,   1,   1,   0,   0,   0,   0,   3,   1,   0,   0,   0,   0,   5,   1,   0,   0,   0,
      0,   7,   1,   0,   0,   0,   0,   9,   1,   0,   0,   0,   0,   11,  1,   0,   0,   0,   0,   13,  1,   0,   0,   0,
      0,   15,  1,   0,   0,   0,   0,   17,  1,   0,   0,   0,   0,   19,  1,   0,   0,   0,   0,   21,  1,   0,   0,   0,
      0,   23,  1,   0,   0,   0,   0,   25,  1,   0,   0,   0,   0,   27,  1,   0,   0,   0,   0,   29,  1,   0,   0,   0,
      0,   31,  1,   0,   0,   0,   0,   33,  1,   0,   0,   0,   0,   35,  1,   0,   0,   0,   0,   37,  1,   0,   0,   0,
      0,   39,  1,   0,   0,   0,   0,   41,  1,   0,   0,   0,   0,   43,  1,   0,   0,   0,   0,   45,  1,   0,   0,   0,
      0,   47,  1,   0,   0,   0,   0,   49,  1,   0,   0,   0,   0,   51,  1,   0,   0,   0,   0,   53,  1,   0,   0,   0,
      0,   55,  1,   0,   0,   0,   0,   57,  1,   0,   0,   0,   0,   59,  1,   0,   0,   0,   0,   61,  1,   0,   0,   0,
      0,   63,  1,   0,   0,   0,   0,   65,  1,   0,   0,   0,   0,   67,  1,   0,   0,   0,   0,   69,  1,   0,   0,   0,
      0,   71,  1,   0,   0,   0,   0,   73,  1,   0,   0,   0,   0,   75,  1,   0,   0,   0,   0,   77,  1,   0,   0,   0,
      0,   79,  1,   0,   0,   0,   0,   81,  1,   0,   0,   0,   0,   83,  1,   0,   0,   0,   0,   85,  1,   0,   0,   0,
      0,   87,  1,   0,   0,   0,   0,   89,  1,   0,   0,   0,   0,   91,  1,   0,   0,   0,   0,   93,  1,   0,   0,   0,
      0,   95,  1,   0,   0,   0,   0,   97,  1,   0,   0,   0,   0,   99,  1,   0,   0,   0,   0,   101, 1,   0,   0,   0,
      0,   103, 1,   0,   0,   0,   0,   105, 1,   0,   0,   0,   0,   107, 1,   0,   0,   0,   0,   109, 1,   0,   0,   0,
      0,   111, 1,   0,   0,   0,   0,   113, 1,   0,   0,   0,   0,   115, 1,   0,   0,   0,   0,   117, 1,   0,   0,   0,
      0,   119, 1,   0,   0,   0,   0,   121, 1,   0,   0,   0,   0,   123, 1,   0,   0,   0,   0,   125, 1,   0,   0,   0,
      0,   127, 1,   0,   0,   0,   0,   129, 1,   0,   0,   0,   0,   131, 1,   0,   0,   0,   0,   133, 1,   0,   0,   0,
      0,   135, 1,   0,   0,   0,   0,   137, 1,   0,   0,   0,   1,   147, 1,   0,   0,   0,   3,   149, 1,   0,   0,   0,
      5,   151, 1,   0,   0,   0,   7,   153, 1,   0,   0,   0,   9,   155, 1,   0,   0,   0,   11,  160, 1,   0,   0,   0,
      13,  165, 1,   0,   0,   0,   15,  169, 1,   0,   0,   0,   17,  175, 1,   0,   0,   0,   19,  180, 1,   0,   0,   0,
      21,  184, 1,   0,   0,   0,   23,  191, 1,   0,   0,   0,   25,  200, 1,   0,   0,   0,   27,  212, 1,   0,   0,   0,
      29,  222, 1,   0,   0,   0,   31,  230, 1,   0,   0,   0,   33,  236, 1,   0,   0,   0,   35,  240, 1,   0,   0,   0,
      37,  244, 1,   0,   0,   0,   39,  251, 1,   0,   0,   0,   41,  277, 1,   0,   0,   0,   43,  291, 1,   0,   0,   0,
      45,  301, 1,   0,   0,   0,   47,  317, 1,   0,   0,   0,   49,  329, 1,   0,   0,   0,   51,  343, 1,   0,   0,   0,
      53,  357, 1,   0,   0,   0,   55,  371, 1,   0,   0,   0,   57,  381, 1,   0,   0,   0,   59,  397, 1,   0,   0,   0,
      61,  417, 1,   0,   0,   0,   63,  427, 1,   0,   0,   0,   65,  443, 1,   0,   0,   0,   67,  451, 1,   0,   0,   0,
      69,  459, 1,   0,   0,   0,   71,  467, 1,   0,   0,   0,   73,  477, 1,   0,   0,   0,   75,  479, 1,   0,   0,   0,
      77,  486, 1,   0,   0,   0,   79,  497, 1,   0,   0,   0,   81,  500, 1,   0,   0,   0,   83,  506, 1,   0,   0,   0,
      85,  517, 1,   0,   0,   0,   87,  519, 1,   0,   0,   0,   89,  521, 1,   0,   0,   0,   91,  523, 1,   0,   0,   0,
      93,  525, 1,   0,   0,   0,   95,  528, 1,   0,   0,   0,   97,  530, 1,   0,   0,   0,   99,  532, 1,   0,   0,   0,
      101, 534, 1,   0,   0,   0,   103, 536, 1,   0,   0,   0,   105, 538, 1,   0,   0,   0,   107, 540, 1,   0,   0,   0,
      109, 542, 1,   0,   0,   0,   111, 544, 1,   0,   0,   0,   113, 546, 1,   0,   0,   0,   115, 548, 1,   0,   0,   0,
      117, 551, 1,   0,   0,   0,   119, 553, 1,   0,   0,   0,   121, 555, 1,   0,   0,   0,   123, 557, 1,   0,   0,   0,
      125, 559, 1,   0,   0,   0,   127, 561, 1,   0,   0,   0,   129, 563, 1,   0,   0,   0,   131, 566, 1,   0,   0,   0,
      133, 572, 1,   0,   0,   0,   135, 587, 1,   0,   0,   0,   137, 598, 1,   0,   0,   0,   139, 609, 1,   0,   0,   0,
      141, 635, 1,   0,   0,   0,   143, 637, 1,   0,   0,   0,   145, 639, 1,   0,   0,   0,   147, 148, 5,   91,  0,   0,
      148, 2,   1,   0,   0,   0,   149, 150, 5,   93,  0,   0,   150, 4,   1,   0,   0,   0,   151, 152, 5,   40,  0,   0,
      152, 6,   1,   0,   0,   0,   153, 154, 5,   41,  0,   0,   154, 8,   1,   0,   0,   0,   155, 156, 5,   83,  0,   0,
      156, 157, 5,   113, 0,   0,   157, 158, 5,   114, 0,   0,   158, 159, 5,   116, 0,   0,   159, 10,  1,   0,   0,   0,
      160, 161, 5,   67,  0,   0,   161, 162, 5,   101, 0,   0,   162, 163, 5,   105, 0,   0,   163, 164, 5,   108, 0,   0,
      164, 12,  1,   0,   0,   0,   165, 166, 5,   65,  0,   0,   166, 167, 5,   98,  0,   0,   167, 168, 5,   115, 0,   0,
      168, 14,  1,   0,   0,   0,   169, 170, 5,   70,  0,   0,   170, 171, 5,   108, 0,   0,   171, 172, 5,   111, 0,   0,
      172, 173, 5,   111, 0,   0,   173, 174, 5,   114, 0,   0,   174, 16,  1,   0,   0,   0,   175, 176, 5,   83,  0,   0,
      176, 177, 5,   105, 0,   0,   177, 178, 5,   103, 0,   0,   178, 179, 5,   110, 0,   0,   179, 18,  1,   0,   0,   0,
      180, 181, 5,   67,  0,   0,   181, 182, 5,   104, 0,   0,   182, 183, 5,   114, 0,   0,   183, 20,  1,   0,   0,   0,
      184, 185, 5,   76,  0,   0,   185, 186, 5,   101, 0,   0,   186, 187, 5,   110, 0,   0,   187, 188, 5,   103, 0,   0,
      188, 189, 5,   116, 0,   0,   189, 190, 5,   104, 0,   0,   190, 22,  1,   0,   0,   0,   191, 192, 5,   84,  0,   0,
      192, 193, 5,   111, 0,   0,   193, 194, 5,   78,  0,   0,   194, 195, 5,   117, 0,   0,   195, 196, 5,   109, 0,   0,
      196, 197, 5,   98,  0,   0,   197, 198, 5,   101, 0,   0,   198, 199, 5,   114, 0,   0,   199, 24,  1,   0,   0,   0,
      200, 201, 5,   84,  0,   0,   201, 202, 5,   111, 0,   0,   202, 203, 5,   84,  0,   0,   203, 204, 5,   105, 0,   0,
      204, 205, 5,   109, 0,   0,   205, 206, 5,   101, 0,   0,   206, 207, 5,   83,  0,   0,   207, 208, 5,   116, 0,   0,
      208, 209, 5,   97,  0,   0,   209, 210, 5,   109, 0,   0,   210, 211, 5,   112, 0,   0,   211, 26,  1,   0,   0,   0,
      212, 213, 5,   70,  0,   0,   213, 214, 5,   108, 0,   0,   214, 215, 5,   111, 0,   0,   215, 216, 5,   97,  0,   0,
      216, 217, 5,   116, 0,   0,   217, 218, 5,   67,  0,   0,   218, 219, 5,   97,  0,   0,   219, 220, 5,   115, 0,   0,
      220, 221, 5,   116, 0,   0,   221, 28,  1,   0,   0,   0,   222, 223, 5,   73,  0,   0,   223, 224, 5,   110, 0,   0,
      224, 225, 5,   116, 0,   0,   225, 226, 5,   67,  0,   0,   226, 227, 5,   97,  0,   0,   227, 228, 5,   115, 0,   0,
      228, 229, 5,   116, 0,   0,   229, 30,  1,   0,   0,   0,   230, 231, 5,   67,  0,   0,   231, 232, 5,   111, 0,   0,
      232, 233, 5,   117, 0,   0,   233, 234, 5,   110, 0,   0,   234, 235, 5,   116, 0,   0,   235, 32,  1,   0,   0,   0,
      236, 237, 5,   67,  0,   0,   237, 238, 5,   114, 0,   0,   238, 239, 5,   99,  0,   0,   239, 34,  1,   0,   0,   0,
      240, 241, 5,   83,  0,   0,   241, 242, 5,   117, 0,   0,   242, 243, 5,   109, 0,   0,   243, 36,  1,   0,   0,   0,
      244, 245, 5,   73,  0,   0,   245, 246, 5,   115, 0,   0,   246, 247, 5,   90,  0,   0,   247, 248, 5,   101, 0,   0,
      248, 249, 5,   114, 0,   0,   249, 250, 5,   111, 0,   0,   250, 38,  1,   0,   0,   0,   251, 252, 5,   73,  0,   0,
      252, 253, 5,   115, 0,   0,   253, 254, 5,   78,  0,   0,   254, 255, 5,   111, 0,   0,   255, 256, 5,   110, 0,   0,
      256, 257, 5,   90,  0,   0,   257, 258, 5,   101, 0,   0,   258, 259, 5,   114, 0,   0,   259, 260, 5,   111, 0,   0,
      260, 40,  1,   0,   0,   0,   261, 262, 5,   66,  0,   0,   262, 263, 5,   89,  0,   0,   263, 264, 5,   84,  0,   0,
      264, 278, 5,   69,  0,   0,   265, 266, 5,   66,  0,   0,   266, 267, 5,   121, 0,   0,   267, 268, 5,   116, 0,   0,
      268, 278, 5,   101, 0,   0,   269, 270, 5,   67,  0,   0,   270, 271, 5,   72,  0,   0,   271, 272, 5,   65,  0,   0,
      272, 278, 5,   82,  0,   0,   273, 274, 5,   67,  0,   0,   274, 275, 5,   104, 0,   0,   275, 276, 5,   97,  0,   0,
      276, 278, 5,   114, 0,   0,   277, 261, 1,   0,   0,   0,   277, 265, 1,   0,   0,   0,   277, 269, 1,   0,   0,   0,
      277, 273, 1,   0,   0,   0,   278, 42,  1,   0,   0,   0,   279, 280, 5,   83,  0,   0,   280, 281, 5,   84,  0,   0,
      281, 282, 5,   82,  0,   0,   282, 283, 5,   73,  0,   0,   283, 284, 5,   78,  0,   0,   284, 292, 5,   71,  0,   0,
      285, 286, 5,   83,  0,   0,   286, 287, 5,   116, 0,   0,   287, 288, 5,   114, 0,   0,   288, 289, 5,   105, 0,   0,
      289, 290, 5,   110, 0,   0,   290, 292, 5,   103, 0,   0,   291, 279, 1,   0,   0,   0,   291, 285, 1,   0,   0,   0,
      292, 44,  1,   0,   0,   0,   293, 294, 5,   85,  0,   0,   294, 295, 5,   73,  0,   0,   295, 296, 5,   78,  0,   0,
      296, 302, 5,   84,  0,   0,   297, 298, 5,   85,  0,   0,   298, 299, 5,   105, 0,   0,   299, 300, 5,   110, 0,   0,
      300, 302, 5,   116, 0,   0,   301, 293, 1,   0,   0,   0,   301, 297, 1,   0,   0,   0,   302, 46,  1,   0,   0,   0,
      303, 304, 5,   73,  0,   0,   304, 305, 5,   78,  0,   0,   305, 306, 5,   84,  0,   0,   306, 307, 5,   69,  0,   0,
      307, 308, 5,   71,  0,   0,   308, 309, 5,   69,  0,   0,   309, 318, 5,   82,  0,   0,   310, 311, 5,   73,  0,   0,
      311, 312, 5,   110, 0,   0,   312, 313, 5,   116, 0,   0,   313, 314, 5,   101, 0,   0,   314, 315, 5,   103, 0,   0,
      315, 316, 5,   101, 0,   0,   316, 318, 5,   114, 0,   0,   317, 303, 1,   0,   0,   0,   317, 310, 1,   0,   0,   0,
      318, 48,  1,   0,   0,   0,   319, 320, 5,   70,  0,   0,   320, 321, 5,   76,  0,   0,   321, 322, 5,   79,  0,   0,
      322, 323, 5,   65,  0,   0,   323, 330, 5,   84,  0,   0,   324, 325, 5,   70,  0,   0,   325, 326, 5,   108, 0,   0,
      326, 327, 5,   111, 0,   0,   327, 328, 5,   97,  0,   0,   328, 330, 5,   116, 0,   0,   329, 319, 1,   0,   0,   0,
      329, 324, 1,   0,   0,   0,   330, 50,  1,   0,   0,   0,   331, 332, 5,   68,  0,   0,   332, 333, 5,   79,  0,   0,
      333, 334, 5,   85,  0,   0,   334, 335, 5,   66,  0,   0,   335, 336, 5,   76,  0,   0,   336, 344, 5,   69,  0,   0,
      337, 338, 5,   68,  0,   0,   338, 339, 5,   111, 0,   0,   339, 340, 5,   117, 0,   0,   340, 341, 5,   98,  0,   0,
      341, 342, 5,   108, 0,   0,   342, 344, 5,   101, 0,   0,   343, 331, 1,   0,   0,   0,   343, 337, 1,   0,   0,   0,
      344, 52,  1,   0,   0,   0,   345, 346, 5,   83,  0,   0,   346, 347, 5,   69,  0,   0,   347, 348, 5,   76,  0,   0,
      348, 349, 5,   69,  0,   0,   349, 350, 5,   67,  0,   0,   350, 358, 5,   84,  0,   0,   351, 352, 5,   115, 0,   0,
      352, 353, 5,   101, 0,   0,   353, 354, 5,   108, 0,   0,   354, 355, 5,   101, 0,   0,   355, 356, 5,   99,  0,   0,
      356, 358, 5,   116, 0,   0,   357, 345, 1,   0,   0,   0,   357, 351, 1,   0,   0,   0,   358, 54,  1,   0,   0,   0,
      359, 360, 5,   83,  0,   0,   360, 361, 5,   84,  0,   0,   361, 362, 5,   82,  0,   0,   362, 363, 5,   69,  0,   0,
      363, 364, 5,   65,  0,   0,   364, 372, 5,   77,  0,   0,   365, 366, 5,   115, 0,   0,   366, 367, 5,   116, 0,   0,
      367, 368, 5,   114, 0,   0,   368, 369, 5,   101, 0,   0,   369, 370, 5,   97,  0,   0,   370, 372, 5,   109, 0,   0,
      371, 359, 1,   0,   0,   0,   371, 365, 1,   0,   0,   0,   372, 56,  1,   0,   0,   0,   373, 374, 5,   70,  0,   0,
      374, 375, 5,   82,  0,   0,   375, 376, 5,   79,  0,   0,   376, 382, 5,   77,  0,   0,   377, 378, 5,   102, 0,   0,
      378, 379, 5,   114, 0,   0,   379, 380, 5,   111, 0,   0,   380, 382, 5,   109, 0,   0,   381, 373, 1,   0,   0,   0,
      381, 377, 1,   0,   0,   0,   382, 58,  1,   0,   0,   0,   383, 384, 5,   68,  0,   0,   384, 385, 5,   69,  0,   0,
      385, 386, 5,   67,  0,   0,   386, 387, 5,   76,  0,   0,   387, 388, 5,   65,  0,   0,   388, 389, 5,   82,  0,   0,
      389, 398, 5,   69,  0,   0,   390, 391, 5,   100, 0,   0,   391, 392, 5,   101, 0,   0,   392, 393, 5,   99,  0,   0,
      393, 394, 5,   108, 0,   0,   394, 395, 5,   97,  0,   0,   395, 396, 5,   114, 0,   0,   396, 398, 5,   101, 0,   0,
      397, 383, 1,   0,   0,   0,   397, 390, 1,   0,   0,   0,   398, 60,  1,   0,   0,   0,   399, 400, 5,   82,  0,   0,
      400, 401, 5,   69,  0,   0,   401, 402, 5,   84,  0,   0,   402, 403, 5,   69,  0,   0,   403, 404, 5,   78,  0,   0,
      404, 405, 5,   84,  0,   0,   405, 406, 5,   73,  0,   0,   406, 407, 5,   79,  0,   0,   407, 418, 5,   78,  0,   0,
      408, 409, 5,   114, 0,   0,   409, 410, 5,   101, 0,   0,   410, 411, 5,   116, 0,   0,   411, 412, 5,   101, 0,   0,
      412, 413, 5,   110, 0,   0,   413, 414, 5,   116, 0,   0,   414, 415, 5,   105, 0,   0,   415, 416, 5,   111, 0,   0,
      416, 418, 5,   110, 0,   0,   417, 399, 1,   0,   0,   0,   417, 408, 1,   0,   0,   0,   418, 62,  1,   0,   0,   0,
      419, 420, 5,   70,  0,   0,   420, 421, 5,   73,  0,   0,   421, 422, 5,   76,  0,   0,   422, 428, 5,   69,  0,   0,
      423, 424, 5,   102, 0,   0,   424, 425, 5,   105, 0,   0,   425, 426, 5,   108, 0,   0,   426, 428, 5,   101, 0,   0,
      427, 419, 1,   0,   0,   0,   427, 423, 1,   0,   0,   0,   428, 64,  1,   0,   0,   0,   429, 430, 5,   83,  0,   0,
      430, 431, 5,   84,  0,   0,   431, 432, 5,   79,  0,   0,   432, 433, 5,   82,  0,   0,   433, 434, 5,   65,  0,   0,
      434, 435, 5,   71,  0,   0,   435, 444, 5,   69,  0,   0,   436, 437, 5,   115, 0,   0,   437, 438, 5,   116, 0,   0,
      438, 439, 5,   111, 0,   0,   439, 440, 5,   114, 0,   0,   440, 441, 5,   97,  0,   0,   441, 442, 5,   103, 0,   0,
      442, 444, 5,   101, 0,   0,   443, 429, 1,   0,   0,   0,   443, 436, 1,   0,   0,   0,   444, 66,  1,   0,   0,   0,
      445, 446, 5,   77,  0,   0,   446, 447, 5,   73,  0,   0,   447, 452, 5,   78,  0,   0,   448, 449, 5,   109, 0,   0,
      449, 450, 5,   105, 0,   0,   450, 452, 5,   110, 0,   0,   451, 445, 1,   0,   0,   0,   451, 448, 1,   0,   0,   0,
      452, 68,  1,   0,   0,   0,   453, 454, 5,   77,  0,   0,   454, 455, 5,   65,  0,   0,   455, 460, 5,   88,  0,   0,
      456, 457, 5,   109, 0,   0,   457, 458, 5,   97,  0,   0,   458, 460, 5,   120, 0,   0,   459, 453, 1,   0,   0,   0,
      459, 456, 1,   0,   0,   0,   460, 70,  1,   0,   0,   0,   461, 462, 5,   65,  0,   0,   462, 463, 5,   86,  0,   0,
      463, 468, 5,   71,  0,   0,   464, 465, 5,   97,  0,   0,   465, 466, 5,   118, 0,   0,   466, 468, 5,   103, 0,   0,
      467, 461, 1,   0,   0,   0,   467, 464, 1,   0,   0,   0,   468, 72,  1,   0,   0,   0,   469, 470, 5,   83,  0,   0,
      470, 471, 5,   85,  0,   0,   471, 472, 5,   77,  0,   0,   472, 478, 5,   67,  0,   0,   473, 474, 5,   115, 0,   0,
      474, 475, 5,   117, 0,   0,   475, 476, 5,   109, 0,   0,   476, 478, 5,   99,  0,   0,   477, 469, 1,   0,   0,   0,
      477, 473, 1,   0,   0,   0,   478, 74,  1,   0,   0,   0,   479, 483, 7,   0,   0,   0,   480, 482, 7,   1,   0,   0,
      481, 480, 1,   0,   0,   0,   482, 485, 1,   0,   0,   0,   483, 481, 1,   0,   0,   0,   483, 484, 1,   0,   0,   0,
      484, 76,  1,   0,   0,   0,   485, 483, 1,   0,   0,   0,   486, 492, 5,   39,  0,   0,   487, 491, 8,   2,   0,   0,
      488, 489, 5,   39,  0,   0,   489, 491, 5,   39,  0,   0,   490, 487, 1,   0,   0,   0,   490, 488, 1,   0,   0,   0,
      491, 494, 1,   0,   0,   0,   492, 490, 1,   0,   0,   0,   492, 493, 1,   0,   0,   0,   493, 495, 1,   0,   0,   0,
      494, 492, 1,   0,   0,   0,   495, 496, 5,   39,  0,   0,   496, 78,  1,   0,   0,   0,   497, 498, 3,   141, 70,  0,
      498, 80,  1,   0,   0,   0,   499, 501, 3,   145, 72,  0,   500, 499, 1,   0,   0,   0,   501, 502, 1,   0,   0,   0,
      502, 500, 1,   0,   0,   0,   502, 503, 1,   0,   0,   0,   503, 82,  1,   0,   0,   0,   504, 507, 3,   81,  40,  0,
      505, 507, 3,   141, 70,  0,   506, 504, 1,   0,   0,   0,   506, 505, 1,   0,   0,   0,   507, 508, 1,   0,   0,   0,
      508, 510, 5,   69,  0,   0,   509, 511, 7,   3,   0,   0,   510, 509, 1,   0,   0,   0,   510, 511, 1,   0,   0,   0,
      511, 513, 1,   0,   0,   0,   512, 514, 3,   145, 72,  0,   513, 512, 1,   0,   0,   0,   514, 515, 1,   0,   0,   0,
      515, 513, 1,   0,   0,   0,   515, 516, 1,   0,   0,   0,   516, 84,  1,   0,   0,   0,   517, 518, 5,   61,  0,   0,
      518, 86,  1,   0,   0,   0,   519, 520, 5,   62,  0,   0,   520, 88,  1,   0,   0,   0,   521, 522, 5,   60,  0,   0,
      522, 90,  1,   0,   0,   0,   523, 524, 5,   33,  0,   0,   524, 92,  1,   0,   0,   0,   525, 526, 5,   124, 0,   0,
      526, 527, 5,   124, 0,   0,   527, 94,  1,   0,   0,   0,   528, 529, 5,   46,  0,   0,   529, 96,  1,   0,   0,   0,
      530, 531, 5,   95,  0,   0,   531, 98,  1,   0,   0,   0,   532, 533, 5,   64,  0,   0,   533, 100, 1,   0,   0,   0,
      534, 535, 5,   35,  0,   0,   535, 102, 1,   0,   0,   0,   536, 537, 5,   38,  0,   0,   537, 104, 1,   0,   0,   0,
      538, 539, 5,   37,  0,   0,   539, 106, 1,   0,   0,   0,   540, 541, 5,   36,  0,   0,   541, 108, 1,   0,   0,   0,
      542, 543, 5,   44,  0,   0,   543, 110, 1,   0,   0,   0,   544, 545, 5,   59,  0,   0,   545, 112, 1,   0,   0,   0,
      546, 547, 5,   58,  0,   0,   547, 114, 1,   0,   0,   0,   548, 549, 5,   58,  0,   0,   549, 550, 5,   58,  0,   0,
      550, 116, 1,   0,   0,   0,   551, 552, 5,   42,  0,   0,   552, 118, 1,   0,   0,   0,   553, 554, 5,   47,  0,   0,
      554, 120, 1,   0,   0,   0,   555, 556, 5,   43,  0,   0,   556, 122, 1,   0,   0,   0,   557, 558, 5,   45,  0,   0,
      558, 124, 1,   0,   0,   0,   559, 560, 5,   126, 0,   0,   560, 126, 1,   0,   0,   0,   561, 562, 5,   124, 0,   0,
      562, 128, 1,   0,   0,   0,   563, 564, 5,   94,  0,   0,   564, 130, 1,   0,   0,   0,   565, 567, 7,   4,   0,   0,
      566, 565, 1,   0,   0,   0,   567, 568, 1,   0,   0,   0,   568, 566, 1,   0,   0,   0,   568, 569, 1,   0,   0,   0,
      569, 570, 1,   0,   0,   0,   570, 571, 6,   65,  0,   0,   571, 132, 1,   0,   0,   0,   572, 573, 5,   47,  0,   0,
      573, 574, 5,   42,  0,   0,   574, 579, 1,   0,   0,   0,   575, 578, 3,   133, 66,  0,   576, 578, 9,   0,   0,   0,
      577, 575, 1,   0,   0,   0,   577, 576, 1,   0,   0,   0,   578, 581, 1,   0,   0,   0,   579, 580, 1,   0,   0,   0,
      579, 577, 1,   0,   0,   0,   580, 582, 1,   0,   0,   0,   581, 579, 1,   0,   0,   0,   582, 583, 5,   42,  0,   0,
      583, 584, 5,   47,  0,   0,   584, 585, 1,   0,   0,   0,   585, 586, 6,   66,  1,   0,   586, 134, 1,   0,   0,   0,
      587, 588, 5,   35,  0,   0,   588, 589, 5,   32,  0,   0,   589, 593, 1,   0,   0,   0,   590, 592, 8,   5,   0,   0,
      591, 590, 1,   0,   0,   0,   592, 595, 1,   0,   0,   0,   593, 591, 1,   0,   0,   0,   593, 594, 1,   0,   0,   0,
      594, 596, 1,   0,   0,   0,   595, 593, 1,   0,   0,   0,   596, 597, 6,   67,  1,   0,   597, 136, 1,   0,   0,   0,
      598, 599, 5,   47,  0,   0,   599, 600, 5,   47,  0,   0,   600, 604, 1,   0,   0,   0,   601, 603, 8,   5,   0,   0,
      602, 601, 1,   0,   0,   0,   603, 606, 1,   0,   0,   0,   604, 602, 1,   0,   0,   0,   604, 605, 1,   0,   0,   0,
      605, 607, 1,   0,   0,   0,   606, 604, 1,   0,   0,   0,   607, 608, 6,   68,  1,   0,   608, 138, 1,   0,   0,   0,
      609, 610, 7,   6,   0,   0,   610, 140, 1,   0,   0,   0,   611, 613, 3,   145, 72,  0,   612, 611, 1,   0,   0,   0,
      613, 614, 1,   0,   0,   0,   614, 612, 1,   0,   0,   0,   614, 615, 1,   0,   0,   0,   615, 616, 1,   0,   0,   0,
      616, 618, 5,   46,  0,   0,   617, 619, 3,   145, 72,  0,   618, 617, 1,   0,   0,   0,   619, 620, 1,   0,   0,   0,
      620, 618, 1,   0,   0,   0,   620, 621, 1,   0,   0,   0,   621, 636, 1,   0,   0,   0,   622, 624, 3,   145, 72,  0,
      623, 622, 1,   0,   0,   0,   624, 625, 1,   0,   0,   0,   625, 623, 1,   0,   0,   0,   625, 626, 1,   0,   0,   0,
      626, 627, 1,   0,   0,   0,   627, 628, 5,   46,  0,   0,   628, 636, 1,   0,   0,   0,   629, 631, 5,   46,  0,   0,
      630, 632, 3,   145, 72,  0,   631, 630, 1,   0,   0,   0,   632, 633, 1,   0,   0,   0,   633, 631, 1,   0,   0,   0,
      633, 634, 1,   0,   0,   0,   634, 636, 1,   0,   0,   0,   635, 612, 1,   0,   0,   0,   635, 623, 1,   0,   0,   0,
      635, 629, 1,   0,   0,   0,   636, 142, 1,   0,   0,   0,   637, 638, 7,   7,   0,   0,   638, 144, 1,   0,   0,   0,
      639, 640, 7,   8,   0,   0,   640, 146, 1,   0,   0,   0,   35,  0,   277, 291, 301, 317, 329, 343, 357, 371, 381, 397,
      417, 427, 443, 451, 459, 467, 477, 483, 490, 492, 502, 506, 510, 515, 568, 577, 579, 593, 604, 614, 620, 625, 633, 635,
      2,   6,   0,   0,   0,   1,   0};
  staticData->serializedATN =
      antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) {
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  rqllexerLexerStaticData = staticData.release();
}

}  // namespace

RQLLexer::RQLLexer(CharStream *input) : Lexer(input) {
  RQLLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *rqllexerLexerStaticData->atn, rqllexerLexerStaticData->decisionToDFA,
                                            rqllexerLexerStaticData->sharedContextCache);
}

RQLLexer::~RQLLexer() { delete _interpreter; }

std::string RQLLexer::getGrammarFileName() const { return "RQL.g4"; }

const std::vector<std::string> &RQLLexer::getRuleNames() const { return rqllexerLexerStaticData->ruleNames; }

const std::vector<std::string> &RQLLexer::getChannelNames() const { return rqllexerLexerStaticData->channelNames; }

const std::vector<std::string> &RQLLexer::getModeNames() const { return rqllexerLexerStaticData->modeNames; }

const dfa::Vocabulary &RQLLexer::getVocabulary() const { return rqllexerLexerStaticData->vocabulary; }

antlr4::atn::SerializedATNView RQLLexer::getSerializedATN() const { return rqllexerLexerStaticData->serializedATN; }

const atn::ATN &RQLLexer::getATN() const { return *rqllexerLexerStaticData->atn; }

void RQLLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  rqllexerLexerInitialize();
#else
  ::antlr4::internal::call_once(rqllexerLexerOnceFlag, rqllexerLexerInitialize);
#endif
}
