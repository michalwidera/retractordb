
// Generated from RQL.g4 by ANTLR 4.13.1

#include "RQLParser.h"

#include "RQLListener.h"

using namespace antlrcpp;

using namespace antlr4;

namespace {

struct RQLParserStaticData final {
  RQLParserStaticData(std::vector<std::string> ruleNames, std::vector<std::string> literalNames,
                      std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)),
        literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RQLParserStaticData(const RQLParserStaticData &)            = delete;
  RQLParserStaticData(RQLParserStaticData &&)                 = delete;
  RQLParserStaticData &operator=(const RQLParserStaticData &) = delete;
  RQLParserStaticData &operator=(RQLParserStaticData &&)      = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag rqlParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
    RQLParserStaticData *rqlParserStaticData = nullptr;

void rqlParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (rqlParserStaticData != nullptr) {
    return;
  }
#else
  assert(rqlParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<RQLParserStaticData>(
      std::vector<std::string>{
          "prog",         "storage_statement",   "select_statement",  "declare_statement", "storage_param",
          "rational_se",  "fraction_rule",       "field_declaration", "field_type",        "select_list",
          "field_id",     "unary_op_expression", "asterisk",          "expression",        "expression_factor",
          "term",         "stream_expression",   "stream_term",       "stream_factor",     "agregator",
          "function_call"},
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
                               "COUNT",
                               "SEGMENTS",
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
      4,   1,   70,  272, 2,   0,   7,   0,   2,   1,   7,   1,   2,   2,   7,   2,   2,   3,   7,   3,   2,   4,  7,   4,
      2,   5,   7,   5,   2,   6,   7,   6,   2,   7,   7,   7,   2,   8,   7,   8,   2,   9,   7,   9,   2,   10, 7,   10,
      2,   11,  7,   11,  2,   12,  7,   12,  2,   13,  7,   13,  2,   14,  7,   14,  2,   15,  7,   15,  2,   16, 7,   16,
      2,   17,  7,   17,  2,   18,  7,   18,  2,   19,  7,   19,  2,   20,  7,   20,  1,   0,   1,   0,   1,   0,  4,   0,
      46,  8,   0,   11,  0,   12,  0,   47,  1,   0,   1,   0,   1,   1,   1,   1,   1,   1,   1,   2,   1,   2,  1,   2,
      1,   2,   1,   2,   1,   2,   1,   2,   1,   2,   1,   2,   5,   2,   64,  8,   2,   10,  2,   12,  2,   67, 9,   2,
      3,   2,   69,  8,   2,   1,   3,   1,   3,   1,   3,   1,   3,   5,   3,   75,  8,   3,   10,  3,   12,  3,  78,  9,
      3,   1,   3,   1,   3,   1,   3,   1,   3,   1,   3,   1,   3,   1,   3,   1,   4,   1,   4,   1,   4,   1,  4,   3,
      4,   91,  8,   4,   1,   5,   1,   5,   1,   5,   3,   5,   96,  8,   5,   1,   6,   1,   6,   1,   6,   1,  6,   1,
      7,   1,   7,   1,   7,   1,   7,   1,   7,   3,   7,   107, 8,   7,   1,   8,   1,   8,   1,   8,   1,   8,  1,   8,
      1,   8,   3,   8,   115, 8,   8,   1,   9,   1,   9,   1,   9,   1,   9,   5,   9,   121, 8,   9,   10,  9,  12,  9,
      124, 9,   9,   3,   9,   126, 8,   9,   1,   10,  1,   10,  1,   10,  1,   10,  1,   10,  1,   10,  1,   10, 1,   10,
      1,   10,  1,   10,  1,   10,  1,   10,  3,   10,  140, 8,   10,  1,   11,  1,   11,  1,   11,  1,   11,  3,  11,  146,
      8,   11,  1,   12,  1,   12,  3,   12,  150, 8,   12,  1,   12,  1,   12,  1,   13,  1,   13,  1,   14,  1,  14,  1,
      14,  1,   14,  1,   14,  1,   14,  1,   14,  1,   14,  1,   14,  5,   14,  165, 8,   14,  10,  14,  12,  14, 168, 9,
      14,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  3,   15,  177, 8,   15,  1,   15, 1,   15,
      3,   15,  181, 8,   15,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  3,   15,  189, 8,   15, 1,   15,
      1,   15,  1,   15,  1,   15,  1,   15,  1,   15,  5,   15,  197, 8,   15,  10,  15,  12,  15,  200, 9,   15, 1,   16,
      1,   16,  1,   16,  1,   16,  1,   16,  1,   16,  1,   16,  1,   16,  1,   16,  1,   16,  1,   16,  1,   16, 1,   16,
      3,   16,  215, 8,   16,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,  17,  1,
      17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  3,   17,  235, 8,   17, 1,   17,
      1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  1,   17,  3,   17,  245, 8,   17,  1,   18,  1,  18,  1,
      18,  1,   18,  1,   18,  3,   18,  252, 8,   18,  1,   19,  1,   19,  1,   19,  1,   19,  3,   19,  258, 8,  19,  1,
      20,  1,   20,  1,   20,  1,   20,  1,   20,  5,   20,  265, 8,   20,  10,  20,  12,  20,  268, 9,   20,  1,  20,  1,
      20,  1,   20,  0,   2,   28,  30,  21,  0,   2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,  26, 28,  30,
      32,  34,  36,  38,  40,  0,   2,   1,   0,   62,  63,  1,   0,   5,   20,  300, 0,   45,  1,   0,   0,   0,  2,   51,
      1,   0,   0,   0,   4,   54,  1,   0,   0,   0,   6,   70,  1,   0,   0,   0,   8,   90,  1,   0,   0,   0,  10,  95,
      1,   0,   0,   0,   12,  97,  1,   0,   0,   0,   14,  101, 1,   0,   0,   0,   16,  114, 1,   0,   0,   0,  18,  125,
      1,   0,   0,   0,   20,  139, 1,   0,   0,   0,   22,  145, 1,   0,   0,   0,   24,  149, 1,   0,   0,   0,  26,  153,
      1,   0,   0,   0,   28,  155, 1,   0,   0,   0,   30,  188, 1,   0,   0,   0,   32,  214, 1,   0,   0,   0,  34,  244,
      1,   0,   0,   0,   36,  251, 1,   0,   0,   0,   38,  257, 1,   0,   0,   0,   40,  259, 1,   0,   0,   0,  42,  46,
      3,   4,   2,   0,   43,  46,  3,   6,   3,   0,   44,  46,  3,   2,   1,   0,   45,  42,  1,   0,   0,   0,  45,  43,
      1,   0,   0,   0,   45,  44,  1,   0,   0,   0,   46,  47,  1,   0,   0,   0,   47,  45,  1,   0,   0,   0,  47,  48,
      1,   0,   0,   0,   48,  49,  1,   0,   0,   0,   49,  50,  5,   0,   0,   1,   50,  1,   1,   0,   0,   0,  51,  52,
      5,   34,  0,   0,   52,  53,  5,   40,  0,   0,   53,  3,   1,   0,   0,   0,   54,  55,  5,   27,  0,   0,  55,  56,
      3,   18,  9,   0,   56,  57,  5,   28,  0,   0,   57,  58,  5,   39,  0,   0,   58,  59,  5,   29,  0,   0,  59,  68,
      3,   32,  16,  0,   60,  61,  5,   33,  0,   0,   61,  65,  5,   40,  0,   0,   62,  64,  3,   8,   4,   0,  63,  62,
      1,   0,   0,   0,   64,  67,  1,   0,   0,   0,   65,  63,  1,   0,   0,   0,   65,  66,  1,   0,   0,   0,  66,  69,
      1,   0,   0,   0,   67,  65,  1,   0,   0,   0,   68,  60,  1,   0,   0,   0,   68,  69,  1,   0,   0,   0,  69,  5,
      1,   0,   0,   0,   70,  71,  5,   30,  0,   0,   71,  76,  3,   14,  7,   0,   72,  73,  5,   56,  0,   0,  73,  75,
      3,   14,  7,   0,   74,  72,  1,   0,   0,   0,   75,  78,  1,   0,   0,   0,   76,  74,  1,   0,   0,   0,  76,  77,
      1,   0,   0,   0,   77,  79,  1,   0,   0,   0,   78,  76,  1,   0,   0,   0,   79,  80,  5,   28,  0,   0,  80,  81,
      5,   39,  0,   0,   81,  82,  5,   56,  0,   0,   82,  83,  3,   10,  5,   0,   83,  84,  5,   33,  0,   0,  84,  85,
      5,   40,  0,   0,   85,  7,   1,   0,   0,   0,   86,  87,  5,   31,  0,   0,   87,  91,  5,   42,  0,   0,  88,  89,
      5,   32,  0,   0,   89,  91,  5,   42,  0,   0,   90,  86,  1,   0,   0,   0,   90,  88,  1,   0,   0,   0,  91,  9,
      1,   0,   0,   0,   92,  96,  3,   12,  6,   0,   93,  96,  5,   41,  0,   0,   94,  96,  5,   42,  0,   0,  95,  92,
      1,   0,   0,   0,   95,  93,  1,   0,   0,   0,   95,  94,  1,   0,   0,   0,   96,  11,  1,   0,   0,   0,  97,  98,
      5,   42,  0,   0,   98,  99,  5,   61,  0,   0,   99,  100, 5,   42,  0,   0,   100, 13,  1,   0,   0,   0,  101, 102,
      5,   39,  0,   0,   102, 106, 3,   16,  8,   0,   103, 104, 5,   1,   0,   0,   104, 105, 5,   42,  0,   0,  105, 107,
      5,   2,   0,   0,   106, 103, 1,   0,   0,   0,   106, 107, 1,   0,   0,   0,   107, 15,  1,   0,   0,   0,  108, 115,
      5,   21,  0,   0,   109, 115, 5,   24,  0,   0,   110, 115, 5,   23,  0,   0,   111, 115, 5,   25,  0,   0,  112, 115,
      5,   26,  0,   0,   113, 115, 5,   22,  0,   0,   114, 108, 1,   0,   0,   0,   114, 109, 1,   0,   0,   0,  114, 110,
      1,   0,   0,   0,   114, 111, 1,   0,   0,   0,   114, 112, 1,   0,   0,   0,   114, 113, 1,   0,   0,   0,  115, 17,
      1,   0,   0,   0,   116, 126, 3,   24,  12,  0,   117, 122, 3,   26,  13,  0,   118, 119, 5,   56,  0,   0,  119, 121,
      3,   26,  13,  0,   120, 118, 1,   0,   0,   0,   121, 124, 1,   0,   0,   0,   122, 120, 1,   0,   0,   0,  122, 123,
      1,   0,   0,   0,   123, 126, 1,   0,   0,   0,   124, 122, 1,   0,   0,   0,   125, 116, 1,   0,   0,   0,  125, 117,
      1,   0,   0,   0,   126, 19,  1,   0,   0,   0,   127, 140, 5,   39,  0,   0,   128, 129, 5,   39,  0,   0,  129, 130,
      5,   1,   0,   0,   130, 131, 5,   50,  0,   0,   131, 140, 5,   2,   0,   0,   132, 133, 5,   39,  0,   0,  133, 134,
      5,   49,  0,   0,   134, 140, 5,   39,  0,   0,   135, 136, 5,   39,  0,   0,   136, 137, 5,   1,   0,   0,  137, 138,
      5,   42,  0,   0,   138, 140, 5,   2,   0,   0,   139, 127, 1,   0,   0,   0,   139, 128, 1,   0,   0,   0,  139, 132,
      1,   0,   0,   0,   139, 135, 1,   0,   0,   0,   140, 21,  1,   0,   0,   0,   141, 142, 5,   64,  0,   0,  142, 146,
      3,   26,  13,  0,   143, 144, 7,   0,   0,   0,   144, 146, 3,   26,  13,  0,   145, 141, 1,   0,   0,   0,  145, 143,
      1,   0,   0,   0,   146, 23,  1,   0,   0,   0,   147, 148, 5,   39,  0,   0,   148, 150, 5,   49,  0,   0,  149, 147,
      1,   0,   0,   0,   149, 150, 1,   0,   0,   0,   150, 151, 1,   0,   0,   0,   151, 152, 5,   60,  0,   0,  152, 25,
      1,   0,   0,   0,   153, 154, 3,   28,  14,  0,   154, 27,  1,   0,   0,   0,   155, 156, 6,   14,  -1,  0,  156, 157,
      3,   30,  15,  0,   157, 166, 1,   0,   0,   0,   158, 159, 10,  3,   0,   0,   159, 160, 5,   62,  0,   0,  160, 165,
      3,   28,  14,  4,   161, 162, 10,  2,   0,   0,   162, 163, 5,   63,  0,   0,   163, 165, 3,   28,  14,  3,  164, 158,
      1,   0,   0,   0,   164, 161, 1,   0,   0,   0,   165, 168, 1,   0,   0,   0,   166, 164, 1,   0,   0,   0,  166, 167,
      1,   0,   0,   0,   167, 29,  1,   0,   0,   0,   168, 166, 1,   0,   0,   0,   169, 170, 6,   15,  -1,  0,  170, 189,
      3,   12,  6,   0,   171, 172, 5,   3,   0,   0,   172, 173, 3,   28,  14,  0,   173, 174, 5,   4,   0,   0,  174, 189,
      1,   0,   0,   0,   175, 177, 5,   63,  0,   0,   176, 175, 1,   0,   0,   0,   176, 177, 1,   0,   0,   0,  177, 178,
      1,   0,   0,   0,   178, 189, 5,   41,  0,   0,   179, 181, 5,   63,  0,   0,   180, 179, 1,   0,   0,   0,  180, 181,
      1,   0,   0,   0,   181, 182, 1,   0,   0,   0,   182, 189, 5,   42,  0,   0,   183, 189, 5,   40,  0,   0,  184, 189,
      3,   22,  11,  0,   185, 189, 3,   20,  10,  0,   186, 189, 3,   38,  19,  0,   187, 189, 3,   40,  20,  0,  188, 169,
      1,   0,   0,   0,   188, 171, 1,   0,   0,   0,   188, 176, 1,   0,   0,   0,   188, 180, 1,   0,   0,   0,  188, 183,
      1,   0,   0,   0,   188, 184, 1,   0,   0,   0,   188, 185, 1,   0,   0,   0,   188, 186, 1,   0,   0,   0,  188, 187,
      1,   0,   0,   0,   189, 198, 1,   0,   0,   0,   190, 191, 10,  11,  0,   0,   191, 192, 5,   60,  0,   0,  192, 197,
      3,   30,  15,  12,  193, 194, 10,  10,  0,   0,   194, 195, 5,   61,  0,   0,   195, 197, 3,   30,  15,  11, 196, 190,
      1,   0,   0,   0,   196, 193, 1,   0,   0,   0,   197, 200, 1,   0,   0,   0,   198, 196, 1,   0,   0,   0,  198, 199,
      1,   0,   0,   0,   199, 31,  1,   0,   0,   0,   200, 198, 1,   0,   0,   0,   201, 202, 3,   34,  17,  0,  202, 203,
      5,   45,  0,   0,   203, 204, 5,   42,  0,   0,   204, 215, 1,   0,   0,   0,   205, 206, 3,   34,  17,  0,  206, 207,
      5,   63,  0,   0,   207, 208, 3,   10,  5,   0,   208, 215, 1,   0,   0,   0,   209, 210, 3,   34,  17,  0,  210, 211,
      5,   62,  0,   0,   211, 212, 3,   34,  17,  0,   212, 215, 1,   0,   0,   0,   213, 215, 3,   34,  17,  0,  214, 201,
      1,   0,   0,   0,   214, 205, 1,   0,   0,   0,   214, 209, 1,   0,   0,   0,   214, 213, 1,   0,   0,   0,  215, 33,
      1,   0,   0,   0,   216, 217, 3,   36,  18,  0,   217, 218, 5,   52,  0,   0,   218, 219, 3,   36,  18,  0,  219, 245,
      1,   0,   0,   0,   220, 221, 3,   36,  18,  0,   221, 222, 5,   53,  0,   0,   222, 223, 3,   10,  5,   0,  223, 245,
      1,   0,   0,   0,   224, 225, 3,   36,  18,  0,   225, 226, 5,   54,  0,   0,   226, 227, 3,   10,  5,   0,  227, 245,
      1,   0,   0,   0,   228, 229, 3,   36,  18,  0,   229, 230, 5,   51,  0,   0,   230, 231, 5,   3,   0,   0,  231, 232,
      5,   42,  0,   0,   232, 234, 5,   56,  0,   0,   233, 235, 5,   63,  0,   0,   234, 233, 1,   0,   0,   0,  234, 235,
      1,   0,   0,   0,   235, 236, 1,   0,   0,   0,   236, 237, 5,   42,  0,   0,   237, 238, 5,   4,   0,   0,  238, 245,
      1,   0,   0,   0,   239, 240, 3,   36,  18,  0,   240, 241, 5,   49,  0,   0,   241, 242, 3,   38,  19,  0,  242, 245,
      1,   0,   0,   0,   243, 245, 3,   36,  18,  0,   244, 216, 1,   0,   0,   0,   244, 220, 1,   0,   0,   0,  244, 224,
      1,   0,   0,   0,   244, 228, 1,   0,   0,   0,   244, 239, 1,   0,   0,   0,   244, 243, 1,   0,   0,   0,  245, 35,
      1,   0,   0,   0,   246, 252, 5,   39,  0,   0,   247, 248, 5,   3,   0,   0,   248, 249, 3,   32,  16,  0,  249, 250,
      5,   4,   0,   0,   250, 252, 1,   0,   0,   0,   251, 246, 1,   0,   0,   0,   251, 247, 1,   0,   0,   0,  252, 37,
      1,   0,   0,   0,   253, 258, 5,   35,  0,   0,   254, 258, 5,   36,  0,   0,   255, 258, 5,   37,  0,   0,  256, 258,
      5,   38,  0,   0,   257, 253, 1,   0,   0,   0,   257, 254, 1,   0,   0,   0,   257, 255, 1,   0,   0,   0,  257, 256,
      1,   0,   0,   0,   258, 39,  1,   0,   0,   0,   259, 260, 7,   1,   0,   0,   260, 261, 5,   3,   0,   0,  261, 266,
      3,   28,  14,  0,   262, 263, 5,   56,  0,   0,   263, 265, 3,   28,  14,  0,   264, 262, 1,   0,   0,   0,  265, 268,
      1,   0,   0,   0,   266, 264, 1,   0,   0,   0,   266, 267, 1,   0,   0,   0,   267, 269, 1,   0,   0,   0,  268, 266,
      1,   0,   0,   0,   269, 270, 5,   4,   0,   0,   270, 41,  1,   0,   0,   0,   27,  45,  47,  65,  68,  76, 90,  95,
      106, 114, 122, 125, 139, 145, 149, 164, 166, 176, 180, 188, 196, 198, 214, 234, 244, 251, 257, 266};
  staticData->serializedATN =
      antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) {
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  rqlParserStaticData = staticData.release();
}

}  // namespace

RQLParser::RQLParser(TokenStream *input) : RQLParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

RQLParser::RQLParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  RQLParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *rqlParserStaticData->atn, rqlParserStaticData->decisionToDFA,
                                             rqlParserStaticData->sharedContextCache, options);
}

RQLParser::~RQLParser() { delete _interpreter; }

const atn::ATN &RQLParser::getATN() const { return *rqlParserStaticData->atn; }

std::string RQLParser::getGrammarFileName() const { return "RQL.g4"; }

const std::vector<std::string> &RQLParser::getRuleNames() const { return rqlParserStaticData->ruleNames; }

const dfa::Vocabulary &RQLParser::getVocabulary() const { return rqlParserStaticData->vocabulary; }

antlr4::atn::SerializedATNView RQLParser::getSerializedATN() const { return rqlParserStaticData->serializedATN; }

//----------------- ProgContext ------------------------------------------------------------------

RQLParser::ProgContext::ProgContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode *RQLParser::ProgContext::EOF() { return getToken(RQLParser::EOF, 0); }

std::vector<RQLParser::Select_statementContext *> RQLParser::ProgContext::select_statement() {
  return getRuleContexts<RQLParser::Select_statementContext>();
}

RQLParser::Select_statementContext *RQLParser::ProgContext::select_statement(size_t i) {
  return getRuleContext<RQLParser::Select_statementContext>(i);
}

std::vector<RQLParser::Declare_statementContext *> RQLParser::ProgContext::declare_statement() {
  return getRuleContexts<RQLParser::Declare_statementContext>();
}

RQLParser::Declare_statementContext *RQLParser::ProgContext::declare_statement(size_t i) {
  return getRuleContext<RQLParser::Declare_statementContext>(i);
}

std::vector<RQLParser::Storage_statementContext *> RQLParser::ProgContext::storage_statement() {
  return getRuleContexts<RQLParser::Storage_statementContext>();
}

RQLParser::Storage_statementContext *RQLParser::ProgContext::storage_statement(size_t i) {
  return getRuleContext<RQLParser::Storage_statementContext>(i);
}

size_t RQLParser::ProgContext::getRuleIndex() const { return RQLParser::RuleProg; }

void RQLParser::ProgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterProg(this);
}

void RQLParser::ProgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitProg(this);
}

RQLParser::ProgContext *RQLParser::prog() {
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
    setState(45);
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(45);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(42);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(43);
          declare_statement();
          break;
        }

        case RQLParser::STORAGE: {
          setState(44);
          storage_statement();
          break;
        }

        default:
          throw NoViableAltException(this);
      }
      setState(47);
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 18387828736) != 0));
    setState(49);
    match(RQLParser::EOF);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Storage_statementContext ------------------------------------------------------------------

RQLParser::Storage_statementContext::Storage_statementContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Storage_statementContext::getRuleIndex() const { return RQLParser::RuleStorage_statement; }

void RQLParser::Storage_statementContext::copyFrom(Storage_statementContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- StorageContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::StorageContext::STORAGE() { return getToken(RQLParser::STORAGE, 0); }

tree::TerminalNode *RQLParser::StorageContext::STRING() { return getToken(RQLParser::STRING, 0); }

RQLParser::StorageContext::StorageContext(Storage_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::StorageContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStorage(this);
}
void RQLParser::StorageContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStorage(this);
}
RQLParser::Storage_statementContext *RQLParser::storage_statement() {
  Storage_statementContext *_localctx = _tracker.createInstance<Storage_statementContext>(_ctx, getState());
  enterRule(_localctx, 2, RQLParser::RuleStorage_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::StorageContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(51);
    match(RQLParser::STORAGE);
    setState(52);
    antlrcpp::downCast<StorageContext *>(_localctx)->folder_name = match(RQLParser::STRING);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_statementContext ------------------------------------------------------------------

RQLParser::Select_statementContext::Select_statementContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Select_statementContext::getRuleIndex() const { return RQLParser::RuleSelect_statement; }

void RQLParser::Select_statementContext::copyFrom(Select_statementContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- SelectContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::SelectContext::SELECT() { return getToken(RQLParser::SELECT, 0); }

RQLParser::Select_listContext *RQLParser::SelectContext::select_list() {
  return getRuleContext<RQLParser::Select_listContext>(0);
}

tree::TerminalNode *RQLParser::SelectContext::STREAM() { return getToken(RQLParser::STREAM, 0); }

tree::TerminalNode *RQLParser::SelectContext::FROM() { return getToken(RQLParser::FROM, 0); }

RQLParser::Stream_expressionContext *RQLParser::SelectContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode *RQLParser::SelectContext::ID() { return getToken(RQLParser::ID, 0); }

tree::TerminalNode *RQLParser::SelectContext::FILE() { return getToken(RQLParser::FILE, 0); }

tree::TerminalNode *RQLParser::SelectContext::STRING() { return getToken(RQLParser::STRING, 0); }

std::vector<RQLParser::Storage_paramContext *> RQLParser::SelectContext::storage_param() {
  return getRuleContexts<RQLParser::Storage_paramContext>();
}

RQLParser::Storage_paramContext *RQLParser::SelectContext::storage_param(size_t i) {
  return getRuleContext<RQLParser::Storage_paramContext>(i);
}

RQLParser::SelectContext::SelectContext(Select_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSelect(this);
}
void RQLParser::SelectContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSelect(this);
}
RQLParser::Select_statementContext *RQLParser::select_statement() {
  Select_statementContext *_localctx = _tracker.createInstance<Select_statementContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleSelect_statement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::SelectContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(54);
    match(RQLParser::SELECT);
    setState(55);
    select_list();
    setState(56);
    match(RQLParser::STREAM);
    setState(57);
    antlrcpp::downCast<SelectContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(58);
    match(RQLParser::FROM);
    setState(59);
    stream_expression();
    setState(68);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(60);
      match(RQLParser::FILE);
      setState(61);
      antlrcpp::downCast<SelectContext *>(_localctx)->name = match(RQLParser::STRING);
      setState(65);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == RQLParser::COUNT

             || _la == RQLParser::SEGMENTS) {
        setState(62);
        storage_param();
        setState(67);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Declare_statementContext ------------------------------------------------------------------

RQLParser::Declare_statementContext::Declare_statementContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Declare_statementContext::getRuleIndex() const { return RQLParser::RuleDeclare_statement; }

void RQLParser::Declare_statementContext::copyFrom(Declare_statementContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- DeclareContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::DeclareContext::DECLARE() { return getToken(RQLParser::DECLARE, 0); }

std::vector<RQLParser::Field_declarationContext *> RQLParser::DeclareContext::field_declaration() {
  return getRuleContexts<RQLParser::Field_declarationContext>();
}

RQLParser::Field_declarationContext *RQLParser::DeclareContext::field_declaration(size_t i) {
  return getRuleContext<RQLParser::Field_declarationContext>(i);
}

tree::TerminalNode *RQLParser::DeclareContext::STREAM() { return getToken(RQLParser::STREAM, 0); }

std::vector<tree::TerminalNode *> RQLParser::DeclareContext::COMMA() { return getTokens(RQLParser::COMMA); }

tree::TerminalNode *RQLParser::DeclareContext::COMMA(size_t i) { return getToken(RQLParser::COMMA, i); }

RQLParser::Rational_seContext *RQLParser::DeclareContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

tree::TerminalNode *RQLParser::DeclareContext::FILE() { return getToken(RQLParser::FILE, 0); }

tree::TerminalNode *RQLParser::DeclareContext::ID() { return getToken(RQLParser::ID, 0); }

tree::TerminalNode *RQLParser::DeclareContext::STRING() { return getToken(RQLParser::STRING, 0); }

RQLParser::DeclareContext::DeclareContext(Declare_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::DeclareContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterDeclare(this);
}
void RQLParser::DeclareContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitDeclare(this);
}
RQLParser::Declare_statementContext *RQLParser::declare_statement() {
  Declare_statementContext *_localctx = _tracker.createInstance<Declare_statementContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleDeclare_statement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::DeclareContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(70);
    match(RQLParser::DECLARE);
    setState(71);
    field_declaration();
    setState(76);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(72);
      match(RQLParser::COMMA);
      setState(73);
      field_declaration();
      setState(78);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(79);
    match(RQLParser::STREAM);
    setState(80);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(81);
    match(RQLParser::COMMA);
    setState(82);
    rational_se();
    setState(83);
    match(RQLParser::FILE);
    setState(84);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Storage_paramContext ------------------------------------------------------------------

RQLParser::Storage_paramContext::Storage_paramContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode *RQLParser::Storage_paramContext::COUNT() { return getToken(RQLParser::COUNT, 0); }

tree::TerminalNode *RQLParser::Storage_paramContext::DECIMAL() { return getToken(RQLParser::DECIMAL, 0); }

tree::TerminalNode *RQLParser::Storage_paramContext::SEGMENTS() { return getToken(RQLParser::SEGMENTS, 0); }

size_t RQLParser::Storage_paramContext::getRuleIndex() const { return RQLParser::RuleStorage_param; }

void RQLParser::Storage_paramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStorage_param(this);
}

void RQLParser::Storage_paramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStorage_param(this);
}

RQLParser::Storage_paramContext *RQLParser::storage_param() {
  Storage_paramContext *_localctx = _tracker.createInstance<Storage_paramContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleStorage_param);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(90);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::COUNT: {
        enterOuterAlt(_localctx, 1);
        setState(86);
        match(RQLParser::COUNT);
        setState(87);
        antlrcpp::downCast<Storage_paramContext *>(_localctx)->size = match(RQLParser::DECIMAL);
        break;
      }

      case RQLParser::SEGMENTS: {
        enterOuterAlt(_localctx, 2);
        setState(88);
        match(RQLParser::SEGMENTS);
        setState(89);
        antlrcpp::downCast<Storage_paramContext *>(_localctx)->segments = match(RQLParser::DECIMAL);
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Rational_seContext ------------------------------------------------------------------

RQLParser::Rational_seContext::Rational_seContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Rational_seContext::getRuleIndex() const { return RQLParser::RuleRational_se; }

void RQLParser::Rational_seContext::copyFrom(Rational_seContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- RationalAsDecimalContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::RationalAsDecimalContext::DECIMAL() { return getToken(RQLParser::DECIMAL, 0); }

RQLParser::RationalAsDecimalContext::RationalAsDecimalContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsDecimalContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterRationalAsDecimal(this);
}
void RQLParser::RationalAsDecimalContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitRationalAsDecimal(this);
}
//----------------- RationalAsFloatContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::RationalAsFloatContext::FLOAT() { return getToken(RQLParser::FLOAT, 0); }

RQLParser::RationalAsFloatContext::RationalAsFloatContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterRationalAsFloat(this);
}
void RQLParser::RationalAsFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitRationalAsFloat(this);
}
//----------------- RationalAsFraction_proformaContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext *RQLParser::RationalAsFraction_proformaContext::fraction_rule() {
  return getRuleContext<RQLParser::Fraction_ruleContext>(0);
}

RQLParser::RationalAsFraction_proformaContext::RationalAsFraction_proformaContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsFraction_proformaContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterRationalAsFraction_proforma(this);
}
void RQLParser::RationalAsFraction_proformaContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitRationalAsFraction_proforma(this);
}
RQLParser::Rational_seContext *RQLParser::rational_se() {
  Rational_seContext *_localctx = _tracker.createInstance<Rational_seContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleRational_se);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(95);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx)) {
      case 1: {
        _localctx = _tracker.createInstance<RQLParser::RationalAsFraction_proformaContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(92);
        fraction_rule();
        break;
      }

      case 2: {
        _localctx = _tracker.createInstance<RQLParser::RationalAsFloatContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(93);
        match(RQLParser::FLOAT);
        break;
      }

      case 3: {
        _localctx = _tracker.createInstance<RQLParser::RationalAsDecimalContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(94);
        match(RQLParser::DECIMAL);
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Fraction_ruleContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext::Fraction_ruleContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Fraction_ruleContext::getRuleIndex() const { return RQLParser::RuleFraction_rule; }

void RQLParser::Fraction_ruleContext::copyFrom(Fraction_ruleContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- FractionContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> RQLParser::FractionContext::DECIMAL() { return getTokens(RQLParser::DECIMAL); }

tree::TerminalNode *RQLParser::FractionContext::DECIMAL(size_t i) { return getToken(RQLParser::DECIMAL, i); }

tree::TerminalNode *RQLParser::FractionContext::DIVIDE() { return getToken(RQLParser::DIVIDE, 0); }

RQLParser::FractionContext::FractionContext(Fraction_ruleContext *ctx) { copyFrom(ctx); }

void RQLParser::FractionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterFraction(this);
}
void RQLParser::FractionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitFraction(this);
}
RQLParser::Fraction_ruleContext *RQLParser::fraction_rule() {
  Fraction_ruleContext *_localctx = _tracker.createInstance<Fraction_ruleContext>(_ctx, getState());
  enterRule(_localctx, 12, RQLParser::RuleFraction_rule);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::FractionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(97);
    match(RQLParser::DECIMAL);
    setState(98);
    match(RQLParser::DIVIDE);
    setState(99);
    match(RQLParser::DECIMAL);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_declarationContext ------------------------------------------------------------------

RQLParser::Field_declarationContext::Field_declarationContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Field_declarationContext::getRuleIndex() const { return RQLParser::RuleField_declaration; }

void RQLParser::Field_declarationContext::copyFrom(Field_declarationContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- SingleDeclarationContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::SingleDeclarationContext::ID() { return getToken(RQLParser::ID, 0); }

RQLParser::Field_typeContext *RQLParser::SingleDeclarationContext::field_type() {
  return getRuleContext<RQLParser::Field_typeContext>(0);
}

tree::TerminalNode *RQLParser::SingleDeclarationContext::DECIMAL() { return getToken(RQLParser::DECIMAL, 0); }

RQLParser::SingleDeclarationContext::SingleDeclarationContext(Field_declarationContext *ctx) { copyFrom(ctx); }

void RQLParser::SingleDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSingleDeclaration(this);
}
void RQLParser::SingleDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSingleDeclaration(this);
}
RQLParser::Field_declarationContext *RQLParser::field_declaration() {
  Field_declarationContext *_localctx = _tracker.createInstance<Field_declarationContext>(_ctx, getState());
  enterRule(_localctx, 14, RQLParser::RuleField_declaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::SingleDeclarationContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(101);
    match(RQLParser::ID);
    setState(102);
    field_type();
    setState(106);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(103);
      match(RQLParser::T__0);
      setState(104);
      antlrcpp::downCast<SingleDeclarationContext *>(_localctx)->type_size = match(RQLParser::DECIMAL);
      setState(105);
      match(RQLParser::T__1);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_typeContext ------------------------------------------------------------------

RQLParser::Field_typeContext::Field_typeContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Field_typeContext::getRuleIndex() const { return RQLParser::RuleField_type; }

void RQLParser::Field_typeContext::copyFrom(Field_typeContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- TypeUnsignedContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::TypeUnsignedContext::UNSIGNED_T() { return getToken(RQLParser::UNSIGNED_T, 0); }

RQLParser::TypeUnsignedContext::TypeUnsignedContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeUnsignedContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterTypeUnsigned(this);
}
void RQLParser::TypeUnsignedContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitTypeUnsigned(this);
}
//----------------- TypeIntContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::TypeIntContext::INTEGER_T() { return getToken(RQLParser::INTEGER_T, 0); }

RQLParser::TypeIntContext::TypeIntContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeIntContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterTypeInt(this);
}
void RQLParser::TypeIntContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitTypeInt(this);
}
//----------------- TypeFloatContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::TypeFloatContext::FLOAT_T() { return getToken(RQLParser::FLOAT_T, 0); }

RQLParser::TypeFloatContext::TypeFloatContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterTypeFloat(this);
}
void RQLParser::TypeFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitTypeFloat(this);
}
//----------------- TypeStringContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::TypeStringContext::STRING_T() { return getToken(RQLParser::STRING_T, 0); }

RQLParser::TypeStringContext::TypeStringContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterTypeString(this);
}
void RQLParser::TypeStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitTypeString(this);
}
//----------------- TypeByteContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::TypeByteContext::BYTE_T() { return getToken(RQLParser::BYTE_T, 0); }

RQLParser::TypeByteContext::TypeByteContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeByteContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterTypeByte(this);
}
void RQLParser::TypeByteContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitTypeByte(this);
}
//----------------- TypeDoubleContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::TypeDoubleContext::DOUBLE_T() { return getToken(RQLParser::DOUBLE_T, 0); }

RQLParser::TypeDoubleContext::TypeDoubleContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeDoubleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterTypeDouble(this);
}
void RQLParser::TypeDoubleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitTypeDouble(this);
}
RQLParser::Field_typeContext *RQLParser::field_type() {
  Field_typeContext *_localctx = _tracker.createInstance<Field_typeContext>(_ctx, getState());
  enterRule(_localctx, 16, RQLParser::RuleField_type);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(114);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BYTE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(108);
        match(RQLParser::BYTE_T);
        break;
      }

      case RQLParser::INTEGER_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeIntContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(109);
        match(RQLParser::INTEGER_T);
        break;
      }

      case RQLParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeUnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(110);
        match(RQLParser::UNSIGNED_T);
        break;
      }

      case RQLParser::FLOAT_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeFloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(111);
        match(RQLParser::FLOAT_T);
        break;
      }

      case RQLParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeDoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(112);
        match(RQLParser::DOUBLE_T);
        break;
      }

      case RQLParser::STRING_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeStringContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(113);
        match(RQLParser::STRING_T);
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_listContext ------------------------------------------------------------------

RQLParser::Select_listContext::Select_listContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Select_listContext::getRuleIndex() const { return RQLParser::RuleSelect_list; }

void RQLParser::Select_listContext::copyFrom(Select_listContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- SelectListContext ------------------------------------------------------------------

std::vector<RQLParser::ExpressionContext *> RQLParser::SelectListContext::expression() {
  return getRuleContexts<RQLParser::ExpressionContext>();
}

RQLParser::ExpressionContext *RQLParser::SelectListContext::expression(size_t i) {
  return getRuleContext<RQLParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::SelectListContext::COMMA() { return getTokens(RQLParser::COMMA); }

tree::TerminalNode *RQLParser::SelectListContext::COMMA(size_t i) { return getToken(RQLParser::COMMA, i); }

RQLParser::SelectListContext::SelectListContext(Select_listContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSelectList(this);
}
void RQLParser::SelectListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSelectList(this);
}
//----------------- SelectListFullscanContext ------------------------------------------------------------------

RQLParser::AsteriskContext *RQLParser::SelectListFullscanContext::asterisk() {
  return getRuleContext<RQLParser::AsteriskContext>(0);
}

RQLParser::SelectListFullscanContext::SelectListFullscanContext(Select_listContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectListFullscanContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSelectListFullscan(this);
}
void RQLParser::SelectListFullscanContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSelectListFullscan(this);
}
RQLParser::Select_listContext *RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 18, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(125);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx)) {
      case 1: {
        _localctx = _tracker.createInstance<RQLParser::SelectListFullscanContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(116);
        asterisk();
        break;
      }

      case 2: {
        _localctx = _tracker.createInstance<RQLParser::SelectListContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(117);
        expression();
        setState(122);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == RQLParser::COMMA) {
          setState(118);
          match(RQLParser::COMMA);
          setState(119);
          expression();
          setState(124);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_idContext ------------------------------------------------------------------

RQLParser::Field_idContext::Field_idContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Field_idContext::getRuleIndex() const { return RQLParser::RuleField_id; }

void RQLParser::Field_idContext::copyFrom(Field_idContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- FieldIDUnderlineContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::FieldIDUnderlineContext::UNDERLINE() { return getToken(RQLParser::UNDERLINE, 0); }

tree::TerminalNode *RQLParser::FieldIDUnderlineContext::ID() { return getToken(RQLParser::ID, 0); }

RQLParser::FieldIDUnderlineContext::FieldIDUnderlineContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDUnderlineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterFieldIDUnderline(this);
}
void RQLParser::FieldIDUnderlineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitFieldIDUnderline(this);
}
//----------------- FieldIDTableContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::FieldIDTableContext::ID() { return getToken(RQLParser::ID, 0); }

tree::TerminalNode *RQLParser::FieldIDTableContext::DECIMAL() { return getToken(RQLParser::DECIMAL, 0); }

RQLParser::FieldIDTableContext::FieldIDTableContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDTableContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterFieldIDTable(this);
}
void RQLParser::FieldIDTableContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitFieldIDTable(this);
}
//----------------- FieldIDContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::FieldIDContext::ID() { return getToken(RQLParser::ID, 0); }

RQLParser::FieldIDContext::FieldIDContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterFieldID(this);
}
void RQLParser::FieldIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitFieldID(this);
}
//----------------- FieldIDColumnNameContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::FieldIDColumnNameContext::DOT() { return getToken(RQLParser::DOT, 0); }

std::vector<tree::TerminalNode *> RQLParser::FieldIDColumnNameContext::ID() { return getTokens(RQLParser::ID); }

tree::TerminalNode *RQLParser::FieldIDColumnNameContext::ID(size_t i) { return getToken(RQLParser::ID, i); }

RQLParser::FieldIDColumnNameContext::FieldIDColumnNameContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDColumnNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterFieldIDColumnName(this);
}
void RQLParser::FieldIDColumnNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitFieldIDColumnName(this);
}
RQLParser::Field_idContext *RQLParser::field_id() {
  Field_idContext *_localctx = _tracker.createInstance<Field_idContext>(_ctx, getState());
  enterRule(_localctx, 20, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(139);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
      case 1: {
        _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(127);
        antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
        break;
      }

      case 2: {
        _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(128);
        antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
        setState(129);
        match(RQLParser::T__0);
        setState(130);
        match(RQLParser::UNDERLINE);
        setState(131);
        match(RQLParser::T__1);
        break;
      }

      case 3: {
        _localctx = _tracker.createInstance<RQLParser::FieldIDColumnNameContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(132);
        antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->tablename = match(RQLParser::ID);
        setState(133);
        match(RQLParser::DOT);
        setState(134);
        antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->column_name = match(RQLParser::ID);
        break;
      }

      case 4: {
        _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(135);
        antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
        setState(136);
        match(RQLParser::T__0);
        setState(137);
        antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = match(RQLParser::DECIMAL);
        setState(138);
        match(RQLParser::T__1);
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unary_op_expressionContext ------------------------------------------------------------------

RQLParser::Unary_op_expressionContext::Unary_op_expressionContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode *RQLParser::Unary_op_expressionContext::BIT_NOT() { return getToken(RQLParser::BIT_NOT, 0); }

RQLParser::ExpressionContext *RQLParser::Unary_op_expressionContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}

tree::TerminalNode *RQLParser::Unary_op_expressionContext::PLUS() { return getToken(RQLParser::PLUS, 0); }

tree::TerminalNode *RQLParser::Unary_op_expressionContext::MINUS() { return getToken(RQLParser::MINUS, 0); }

size_t RQLParser::Unary_op_expressionContext::getRuleIndex() const { return RQLParser::RuleUnary_op_expression; }

void RQLParser::Unary_op_expressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterUnary_op_expression(this);
}

void RQLParser::Unary_op_expressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitUnary_op_expression(this);
}

RQLParser::Unary_op_expressionContext *RQLParser::unary_op_expression() {
  Unary_op_expressionContext *_localctx = _tracker.createInstance<Unary_op_expressionContext>(_ctx, getState());
  enterRule(_localctx, 22, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(145);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(141);
        match(RQLParser::BIT_NOT);
        setState(142);
        expression();
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(143);
        antlrcpp::downCast<Unary_op_expressionContext *>(_localctx)->op = _input->LT(1);
        _la                                                             = _input->LA(1);
        if (!(_la == RQLParser::PLUS

              || _la == RQLParser::MINUS)) {
          antlrcpp::downCast<Unary_op_expressionContext *>(_localctx)->op = _errHandler->recoverInline(this);
        } else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(144);
        expression();
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AsteriskContext ------------------------------------------------------------------

RQLParser::AsteriskContext::AsteriskContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode *RQLParser::AsteriskContext::STAR() { return getToken(RQLParser::STAR, 0); }

tree::TerminalNode *RQLParser::AsteriskContext::ID() { return getToken(RQLParser::ID, 0); }

tree::TerminalNode *RQLParser::AsteriskContext::DOT() { return getToken(RQLParser::DOT, 0); }

size_t RQLParser::AsteriskContext::getRuleIndex() const { return RQLParser::RuleAsterisk; }

void RQLParser::AsteriskContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterAsterisk(this);
}

void RQLParser::AsteriskContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitAsterisk(this);
}

RQLParser::AsteriskContext *RQLParser::asterisk() {
  AsteriskContext *_localctx = _tracker.createInstance<AsteriskContext>(_ctx, getState());
  enterRule(_localctx, 24, RQLParser::RuleAsterisk);
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
    setState(149);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(147);
      match(RQLParser::ID);
      setState(148);
      match(RQLParser::DOT);
    }
    setState(151);
    match(RQLParser::STAR);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

RQLParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

RQLParser::Expression_factorContext *RQLParser::ExpressionContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

size_t RQLParser::ExpressionContext::getRuleIndex() const { return RQLParser::RuleExpression; }

void RQLParser::ExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpression(this);
}

void RQLParser::ExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpression(this);
}

RQLParser::ExpressionContext *RQLParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 26, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(153);
    expression_factor(0);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Expression_factorContext ------------------------------------------------------------------

RQLParser::Expression_factorContext::Expression_factorContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Expression_factorContext::getRuleIndex() const { return RQLParser::RuleExpression_factor; }

void RQLParser::Expression_factorContext::copyFrom(Expression_factorContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- ExpPlusContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_factorContext *> RQLParser::ExpPlusContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext *RQLParser::ExpPlusContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

tree::TerminalNode *RQLParser::ExpPlusContext::PLUS() { return getToken(RQLParser::PLUS, 0); }

RQLParser::ExpPlusContext::ExpPlusContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpPlusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpPlus(this);
}
void RQLParser::ExpPlusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpPlus(this);
}
//----------------- ExpMinusContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_factorContext *> RQLParser::ExpMinusContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext *RQLParser::ExpMinusContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

tree::TerminalNode *RQLParser::ExpMinusContext::MINUS() { return getToken(RQLParser::MINUS, 0); }

RQLParser::ExpMinusContext::ExpMinusContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpMinusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpMinus(this);
}
void RQLParser::ExpMinusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpMinus(this);
}
//----------------- ExpTermContext ------------------------------------------------------------------

RQLParser::TermContext *RQLParser::ExpTermContext::term() { return getRuleContext<RQLParser::TermContext>(0); }

RQLParser::ExpTermContext::ExpTermContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpTermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpTerm(this);
}
void RQLParser::ExpTermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpTerm(this);
}

RQLParser::Expression_factorContext *RQLParser::expression_factor() { return expression_factor(0); }

RQLParser::Expression_factorContext *RQLParser::expression_factor(int precedence) {
  ParserRuleContext *parentContext                     = _ctx;
  size_t parentState                                   = getState();
  RQLParser::Expression_factorContext *_localctx       = _tracker.createInstance<Expression_factorContext>(_ctx, parentState);
  RQLParser::Expression_factorContext *previousContext = _localctx;
  (void)previousContext;  // Silence compiler, in case the context is not used by generated code.
  size_t startState = 28;
  enterRecursionRule(_localctx, 28, RQLParser::RuleExpression_factor, precedence);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    _localctx       = _tracker.createInstance<ExpTermContext>(_localctx);
    _ctx            = _localctx;
    previousContext = _localctx;

    setState(156);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(166);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty()) triggerExitRuleEvent();
        previousContext = _localctx;
        setState(164);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 14, _ctx)) {
          case 1: {
            auto newContext = _tracker.createInstance<ExpPlusContext>(
                _tracker.createInstance<Expression_factorContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExpression_factor);
            setState(158);

            if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
            setState(159);
            match(RQLParser::PLUS);
            setState(160);
            expression_factor(4);
            break;
          }

          case 2: {
            auto newContext = _tracker.createInstance<ExpMinusContext>(
                _tracker.createInstance<Expression_factorContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExpression_factor);
            setState(161);

            if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
            setState(162);
            match(RQLParser::MINUS);
            setState(163);
            expression_factor(3);
            break;
          }

          default:
            break;
        }
      }
      setState(168);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx);
    }
  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- TermContext ------------------------------------------------------------------

RQLParser::TermContext::TermContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::TermContext::getRuleIndex() const { return RQLParser::RuleTerm; }

void RQLParser::TermContext::copyFrom(TermContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- ExpInContext ------------------------------------------------------------------

RQLParser::Expression_factorContext *RQLParser::ExpInContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

RQLParser::ExpInContext::ExpInContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpInContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpIn(this);
}
void RQLParser::ExpInContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpIn(this);
}
//----------------- ExpRationalContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext *RQLParser::ExpRationalContext::fraction_rule() {
  return getRuleContext<RQLParser::Fraction_ruleContext>(0);
}

RQLParser::ExpRationalContext::ExpRationalContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpRationalContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpRational(this);
}
void RQLParser::ExpRationalContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpRational(this);
}
//----------------- ExpFloatContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::ExpFloatContext::FLOAT() { return getToken(RQLParser::FLOAT, 0); }

tree::TerminalNode *RQLParser::ExpFloatContext::MINUS() { return getToken(RQLParser::MINUS, 0); }

RQLParser::ExpFloatContext::ExpFloatContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpFloat(this);
}
void RQLParser::ExpFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpFloat(this);
}
//----------------- ExpDecContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::ExpDecContext::DECIMAL() { return getToken(RQLParser::DECIMAL, 0); }

tree::TerminalNode *RQLParser::ExpDecContext::MINUS() { return getToken(RQLParser::MINUS, 0); }

RQLParser::ExpDecContext::ExpDecContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpDecContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpDec(this);
}
void RQLParser::ExpDecContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpDec(this);
}
//----------------- ExpAggContext ------------------------------------------------------------------

RQLParser::AgregatorContext *RQLParser::ExpAggContext::agregator() { return getRuleContext<RQLParser::AgregatorContext>(0); }

RQLParser::ExpAggContext::ExpAggContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpAggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpAgg(this);
}
void RQLParser::ExpAggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpAgg(this);
}
//----------------- ExpFnCallContext ------------------------------------------------------------------

RQLParser::Function_callContext *RQLParser::ExpFnCallContext::function_call() {
  return getRuleContext<RQLParser::Function_callContext>(0);
}

RQLParser::ExpFnCallContext::ExpFnCallContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFnCallContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpFnCall(this);
}
void RQLParser::ExpFnCallContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpFnCall(this);
}
//----------------- ExpDivContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpDivContext::term() { return getRuleContexts<RQLParser::TermContext>(); }

RQLParser::TermContext *RQLParser::ExpDivContext::term(size_t i) { return getRuleContext<RQLParser::TermContext>(i); }

tree::TerminalNode *RQLParser::ExpDivContext::DIVIDE() { return getToken(RQLParser::DIVIDE, 0); }

RQLParser::ExpDivContext::ExpDivContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpDivContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpDiv(this);
}
void RQLParser::ExpDivContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpDiv(this);
}
//----------------- ExpFieldContext ------------------------------------------------------------------

RQLParser::Field_idContext *RQLParser::ExpFieldContext::field_id() { return getRuleContext<RQLParser::Field_idContext>(0); }

RQLParser::ExpFieldContext::ExpFieldContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFieldContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpField(this);
}
void RQLParser::ExpFieldContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpField(this);
}
//----------------- ExpStringContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::ExpStringContext::STRING() { return getToken(RQLParser::STRING, 0); }

RQLParser::ExpStringContext::ExpStringContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpString(this);
}
void RQLParser::ExpStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpString(this);
}
//----------------- ExpUnaryContext ------------------------------------------------------------------

RQLParser::Unary_op_expressionContext *RQLParser::ExpUnaryContext::unary_op_expression() {
  return getRuleContext<RQLParser::Unary_op_expressionContext>(0);
}

RQLParser::ExpUnaryContext::ExpUnaryContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpUnaryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpUnary(this);
}
void RQLParser::ExpUnaryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpUnary(this);
}
//----------------- ExpMultContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpMultContext::term() { return getRuleContexts<RQLParser::TermContext>(); }

RQLParser::TermContext *RQLParser::ExpMultContext::term(size_t i) { return getRuleContext<RQLParser::TermContext>(i); }

tree::TerminalNode *RQLParser::ExpMultContext::STAR() { return getToken(RQLParser::STAR, 0); }

RQLParser::ExpMultContext::ExpMultContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpMultContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterExpMult(this);
}
void RQLParser::ExpMultContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitExpMult(this);
}

RQLParser::TermContext *RQLParser::term() { return term(0); }

RQLParser::TermContext *RQLParser::term(int precedence) {
  ParserRuleContext *parentContext        = _ctx;
  size_t parentState                      = getState();
  RQLParser::TermContext *_localctx       = _tracker.createInstance<TermContext>(_ctx, parentState);
  RQLParser::TermContext *previousContext = _localctx;
  (void)previousContext;  // Silence compiler, in case the context is not used by generated code.
  size_t startState = 30;
  enterRecursionRule(_localctx, 30, RQLParser::RuleTerm, precedence);

  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(188);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
      case 1: {
        _localctx       = _tracker.createInstance<ExpRationalContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;

        setState(170);
        fraction_rule();
        break;
      }

      case 2: {
        _localctx       = _tracker.createInstance<ExpInContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(171);
        match(RQLParser::T__2);
        setState(172);
        expression_factor(0);
        setState(173);
        match(RQLParser::T__3);
        break;
      }

      case 3: {
        _localctx       = _tracker.createInstance<ExpFloatContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(176);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == RQLParser::MINUS) {
          setState(175);
          match(RQLParser::MINUS);
        }
        setState(178);
        match(RQLParser::FLOAT);
        break;
      }

      case 4: {
        _localctx       = _tracker.createInstance<ExpDecContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(180);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == RQLParser::MINUS) {
          setState(179);
          match(RQLParser::MINUS);
        }
        setState(182);
        match(RQLParser::DECIMAL);
        break;
      }

      case 5: {
        _localctx       = _tracker.createInstance<ExpStringContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(183);
        match(RQLParser::STRING);
        break;
      }

      case 6: {
        _localctx       = _tracker.createInstance<ExpUnaryContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(184);
        unary_op_expression();
        break;
      }

      case 7: {
        _localctx       = _tracker.createInstance<ExpFieldContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(185);
        field_id();
        break;
      }

      case 8: {
        _localctx       = _tracker.createInstance<ExpAggContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(186);
        agregator();
        break;
      }

      case 9: {
        _localctx       = _tracker.createInstance<ExpFnCallContext>(_localctx);
        _ctx            = _localctx;
        previousContext = _localctx;
        setState(187);
        function_call();
        break;
      }

      default:
        break;
    }
    _ctx->stop = _input->LT(-1);
    setState(198);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty()) triggerExitRuleEvent();
        previousContext = _localctx;
        setState(196);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
          case 1: {
            auto newContext =
                _tracker.createInstance<ExpMultContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleTerm);
            setState(190);

            if (!(precpred(_ctx, 11))) throw FailedPredicateException(this, "precpred(_ctx, 11)");
            setState(191);
            match(RQLParser::STAR);
            setState(192);
            term(12);
            break;
          }

          case 2: {
            auto newContext =
                _tracker.createInstance<ExpDivContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleTerm);
            setState(193);

            if (!(precpred(_ctx, 10))) throw FailedPredicateException(this, "precpred(_ctx, 10)");
            setState(194);
            match(RQLParser::DIVIDE);
            setState(195);
            term(11);
            break;
          }

          default:
            break;
        }
      }
      setState(200);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx);
    }
  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- Stream_expressionContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext::Stream_expressionContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Stream_expressionContext::getRuleIndex() const { return RQLParser::RuleStream_expression; }

void RQLParser::Stream_expressionContext::copyFrom(Stream_expressionContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- SExpPlusContext ------------------------------------------------------------------

std::vector<RQLParser::Stream_termContext *> RQLParser::SExpPlusContext::stream_term() {
  return getRuleContexts<RQLParser::Stream_termContext>();
}

RQLParser::Stream_termContext *RQLParser::SExpPlusContext::stream_term(size_t i) {
  return getRuleContext<RQLParser::Stream_termContext>(i);
}

tree::TerminalNode *RQLParser::SExpPlusContext::PLUS() { return getToken(RQLParser::PLUS, 0); }

RQLParser::SExpPlusContext::SExpPlusContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpPlusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpPlus(this);
}
void RQLParser::SExpPlusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpPlus(this);
}
//----------------- SExpTermContext ------------------------------------------------------------------

RQLParser::Stream_termContext *RQLParser::SExpTermContext::stream_term() {
  return getRuleContext<RQLParser::Stream_termContext>(0);
}

RQLParser::SExpTermContext::SExpTermContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpTermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpTerm(this);
}
void RQLParser::SExpTermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpTerm(this);
}
//----------------- SExpTimeMoveContext ------------------------------------------------------------------

RQLParser::Stream_termContext *RQLParser::SExpTimeMoveContext::stream_term() {
  return getRuleContext<RQLParser::Stream_termContext>(0);
}

tree::TerminalNode *RQLParser::SExpTimeMoveContext::GREATER() { return getToken(RQLParser::GREATER, 0); }

tree::TerminalNode *RQLParser::SExpTimeMoveContext::DECIMAL() { return getToken(RQLParser::DECIMAL, 0); }

RQLParser::SExpTimeMoveContext::SExpTimeMoveContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpTimeMoveContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpTimeMove(this);
}
void RQLParser::SExpTimeMoveContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpTimeMove(this);
}
//----------------- SExpMinusContext ------------------------------------------------------------------

RQLParser::Stream_termContext *RQLParser::SExpMinusContext::stream_term() {
  return getRuleContext<RQLParser::Stream_termContext>(0);
}

tree::TerminalNode *RQLParser::SExpMinusContext::MINUS() { return getToken(RQLParser::MINUS, 0); }

RQLParser::Rational_seContext *RQLParser::SExpMinusContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpMinusContext::SExpMinusContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpMinusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpMinus(this);
}
void RQLParser::SExpMinusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpMinus(this);
}
RQLParser::Stream_expressionContext *RQLParser::stream_expression() {
  Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, getState());
  enterRule(_localctx, 32, RQLParser::RuleStream_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(214);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx)) {
      case 1: {
        _localctx = _tracker.createInstance<RQLParser::SExpTimeMoveContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(201);
        stream_term();
        setState(202);
        match(RQLParser::GREATER);
        setState(203);
        match(RQLParser::DECIMAL);
        break;
      }

      case 2: {
        _localctx = _tracker.createInstance<RQLParser::SExpMinusContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(205);
        stream_term();
        setState(206);
        match(RQLParser::MINUS);
        setState(207);
        rational_se();
        break;
      }

      case 3: {
        _localctx = _tracker.createInstance<RQLParser::SExpPlusContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(209);
        stream_term();
        setState(210);
        match(RQLParser::PLUS);
        setState(211);
        stream_term();
        break;
      }

      case 4: {
        _localctx = _tracker.createInstance<RQLParser::SExpTermContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(213);
        stream_term();
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Stream_termContext ------------------------------------------------------------------

RQLParser::Stream_termContext::Stream_termContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::Stream_termContext::getRuleIndex() const { return RQLParser::RuleStream_term; }

void RQLParser::Stream_termContext::copyFrom(Stream_termContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- SExpFactorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext *RQLParser::SExpFactorContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

RQLParser::SExpFactorContext::SExpFactorContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpFactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpFactor(this);
}
void RQLParser::SExpFactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpFactor(this);
}
//----------------- SExpHashContext ------------------------------------------------------------------

std::vector<RQLParser::Stream_factorContext *> RQLParser::SExpHashContext::stream_factor() {
  return getRuleContexts<RQLParser::Stream_factorContext>();
}

RQLParser::Stream_factorContext *RQLParser::SExpHashContext::stream_factor(size_t i) {
  return getRuleContext<RQLParser::Stream_factorContext>(i);
}

tree::TerminalNode *RQLParser::SExpHashContext::SHARP() { return getToken(RQLParser::SHARP, 0); }

RQLParser::SExpHashContext::SExpHashContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpHashContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpHash(this);
}
void RQLParser::SExpHashContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpHash(this);
}
//----------------- SExpModContext ------------------------------------------------------------------

RQLParser::Stream_factorContext *RQLParser::SExpModContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode *RQLParser::SExpModContext::MOD() { return getToken(RQLParser::MOD, 0); }

RQLParser::Rational_seContext *RQLParser::SExpModContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpModContext::SExpModContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpModContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpMod(this);
}
void RQLParser::SExpModContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpMod(this);
}
//----------------- SExpAgregate_proformaContext ------------------------------------------------------------------

RQLParser::Stream_factorContext *RQLParser::SExpAgregate_proformaContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode *RQLParser::SExpAgregate_proformaContext::DOT() { return getToken(RQLParser::DOT, 0); }

RQLParser::AgregatorContext *RQLParser::SExpAgregate_proformaContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

RQLParser::SExpAgregate_proformaContext::SExpAgregate_proformaContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAgregate_proformaContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpAgregate_proforma(this);
}
void RQLParser::SExpAgregate_proformaContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpAgregate_proforma(this);
}
//----------------- SExpAgseContext ------------------------------------------------------------------

RQLParser::Stream_factorContext *RQLParser::SExpAgseContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode *RQLParser::SExpAgseContext::AT() { return getToken(RQLParser::AT, 0); }

tree::TerminalNode *RQLParser::SExpAgseContext::COMMA() { return getToken(RQLParser::COMMA, 0); }

std::vector<tree::TerminalNode *> RQLParser::SExpAgseContext::DECIMAL() { return getTokens(RQLParser::DECIMAL); }

tree::TerminalNode *RQLParser::SExpAgseContext::DECIMAL(size_t i) { return getToken(RQLParser::DECIMAL, i); }

tree::TerminalNode *RQLParser::SExpAgseContext::MINUS() { return getToken(RQLParser::MINUS, 0); }

RQLParser::SExpAgseContext::SExpAgseContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAgseContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpAgse(this);
}
void RQLParser::SExpAgseContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpAgse(this);
}
//----------------- SExpAndContext ------------------------------------------------------------------

RQLParser::Stream_factorContext *RQLParser::SExpAndContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode *RQLParser::SExpAndContext::AND() { return getToken(RQLParser::AND, 0); }

RQLParser::Rational_seContext *RQLParser::SExpAndContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpAndContext::SExpAndContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterSExpAnd(this);
}
void RQLParser::SExpAndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitSExpAnd(this);
}
RQLParser::Stream_termContext *RQLParser::stream_term() {
  Stream_termContext *_localctx = _tracker.createInstance<Stream_termContext>(_ctx, getState());
  enterRule(_localctx, 34, RQLParser::RuleStream_term);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(244);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
      case 1: {
        _localctx = _tracker.createInstance<RQLParser::SExpHashContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(216);
        stream_factor();
        setState(217);
        match(RQLParser::SHARP);
        setState(218);
        stream_factor();
        break;
      }

      case 2: {
        _localctx = _tracker.createInstance<RQLParser::SExpAndContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(220);
        stream_factor();
        setState(221);
        match(RQLParser::AND);
        setState(222);
        rational_se();
        break;
      }

      case 3: {
        _localctx = _tracker.createInstance<RQLParser::SExpModContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(224);
        stream_factor();
        setState(225);
        match(RQLParser::MOD);
        setState(226);
        rational_se();
        break;
      }

      case 4: {
        _localctx = _tracker.createInstance<RQLParser::SExpAgseContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(228);
        stream_factor();
        setState(229);
        match(RQLParser::AT);
        setState(230);
        match(RQLParser::T__2);
        setState(231);
        antlrcpp::downCast<SExpAgseContext *>(_localctx)->step = match(RQLParser::DECIMAL);
        setState(232);
        match(RQLParser::COMMA);
        setState(234);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == RQLParser::MINUS) {
          setState(233);
          match(RQLParser::MINUS);
        }
        setState(236);
        antlrcpp::downCast<SExpAgseContext *>(_localctx)->window = match(RQLParser::DECIMAL);
        setState(237);
        match(RQLParser::T__3);
        break;
      }

      case 5: {
        _localctx = _tracker.createInstance<RQLParser::SExpAgregate_proformaContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(239);
        stream_factor();
        setState(240);
        match(RQLParser::DOT);
        setState(241);
        agregator();
        break;
      }

      case 6: {
        _localctx = _tracker.createInstance<RQLParser::SExpFactorContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(243);
        stream_factor();
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Stream_factorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext::Stream_factorContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

tree::TerminalNode *RQLParser::Stream_factorContext::ID() { return getToken(RQLParser::ID, 0); }

RQLParser::Stream_expressionContext *RQLParser::Stream_factorContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

size_t RQLParser::Stream_factorContext::getRuleIndex() const { return RQLParser::RuleStream_factor; }

void RQLParser::Stream_factorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStream_factor(this);
}

void RQLParser::Stream_factorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStream_factor(this);
}

RQLParser::Stream_factorContext *RQLParser::stream_factor() {
  Stream_factorContext *_localctx = _tracker.createInstance<Stream_factorContext>(_ctx, getState());
  enterRule(_localctx, 36, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(251);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::ID: {
        enterOuterAlt(_localctx, 1);
        setState(246);
        match(RQLParser::ID);
        break;
      }

      case RQLParser::T__2: {
        enterOuterAlt(_localctx, 2);
        setState(247);
        match(RQLParser::T__2);
        setState(248);
        stream_expression();
        setState(249);
        match(RQLParser::T__3);
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AgregatorContext ------------------------------------------------------------------

RQLParser::AgregatorContext::AgregatorContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

size_t RQLParser::AgregatorContext::getRuleIndex() const { return RQLParser::RuleAgregator; }

void RQLParser::AgregatorContext::copyFrom(AgregatorContext *ctx) { ParserRuleContext::copyFrom(ctx); }

//----------------- StreamMinContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::StreamMinContext::MIN() { return getToken(RQLParser::MIN, 0); }

RQLParser::StreamMinContext::StreamMinContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamMinContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStreamMin(this);
}
void RQLParser::StreamMinContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStreamMin(this);
}
//----------------- StreamAvgContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::StreamAvgContext::AVG() { return getToken(RQLParser::AVG, 0); }

RQLParser::StreamAvgContext::StreamAvgContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamAvgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStreamAvg(this);
}
void RQLParser::StreamAvgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStreamAvg(this);
}
//----------------- StreamMaxContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::StreamMaxContext::MAX() { return getToken(RQLParser::MAX, 0); }

RQLParser::StreamMaxContext::StreamMaxContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamMaxContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStreamMax(this);
}
void RQLParser::StreamMaxContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStreamMax(this);
}
//----------------- StreamSumContext ------------------------------------------------------------------

tree::TerminalNode *RQLParser::StreamSumContext::SUMC() { return getToken(RQLParser::SUMC, 0); }

RQLParser::StreamSumContext::StreamSumContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamSumContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterStreamSum(this);
}
void RQLParser::StreamSumContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitStreamSum(this);
}
RQLParser::AgregatorContext *RQLParser::agregator() {
  AgregatorContext *_localctx = _tracker.createInstance<AgregatorContext>(_ctx, getState());
  enterRule(_localctx, 38, RQLParser::RuleAgregator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(257);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::MIN: {
        _localctx = _tracker.createInstance<RQLParser::StreamMinContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(253);
        match(RQLParser::MIN);
        break;
      }

      case RQLParser::MAX: {
        _localctx = _tracker.createInstance<RQLParser::StreamMaxContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(254);
        match(RQLParser::MAX);
        break;
      }

      case RQLParser::AVG: {
        _localctx = _tracker.createInstance<RQLParser::StreamAvgContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(255);
        match(RQLParser::AVG);
        break;
      }

      case RQLParser::SUMC: {
        _localctx = _tracker.createInstance<RQLParser::StreamSumContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(256);
        match(RQLParser::SUMC);
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Function_callContext ------------------------------------------------------------------

RQLParser::Function_callContext::Function_callContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {}

std::vector<RQLParser::Expression_factorContext *> RQLParser::Function_callContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext *RQLParser::Function_callContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Function_callContext::COMMA() { return getTokens(RQLParser::COMMA); }

tree::TerminalNode *RQLParser::Function_callContext::COMMA(size_t i) { return getToken(RQLParser::COMMA, i); }

size_t RQLParser::Function_callContext::getRuleIndex() const { return RQLParser::RuleFunction_call; }

void RQLParser::Function_callContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->enterFunction_call(this);
}

void RQLParser::Function_callContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr) parserListener->exitFunction_call(this);
}

RQLParser::Function_callContext *RQLParser::function_call() {
  Function_callContext *_localctx = _tracker.createInstance<Function_callContext>(_ctx, getState());
  enterRule(_localctx, 40, RQLParser::RuleFunction_call);
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
    setState(259);
    _la = _input->LA(1);
    if (!((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 2097120) != 0))) {
      _errHandler->recoverInline(this);
    } else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(260);
    match(RQLParser::T__2);
    setState(261);
    expression_factor(0);
    setState(266);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(262);
      match(RQLParser::COMMA);
      setState(263);
      expression_factor(0);
      setState(268);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(269);
    match(RQLParser::T__3);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool RQLParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 14:
      return expression_factorSempred(antlrcpp::downCast<Expression_factorContext *>(context), predicateIndex);
    case 15:
      return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);

    default:
      break;
  }
  return true;
}

bool RQLParser::expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0:
      return precpred(_ctx, 3);
    case 1:
      return precpred(_ctx, 2);

    default:
      break;
  }
  return true;
}

bool RQLParser::termSempred(TermContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2:
      return precpred(_ctx, 11);
    case 3:
      return precpred(_ctx, 10);

    default:
      break;
  }
  return true;
}

void RQLParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  rqlParserInitialize();
#else
  ::antlr4::internal::call_once(rqlParserOnceFlag, rqlParserInitialize);
#endif
}
