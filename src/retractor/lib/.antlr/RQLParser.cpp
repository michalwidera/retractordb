
// Generated from RQL.g4 by ANTLR 4.13.1


#include "RQLListener.h"

#include "RQLParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct RQLParserStaticData final {
  RQLParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RQLParserStaticData(const RQLParserStaticData&) = delete;
  RQLParserStaticData(RQLParserStaticData&&) = delete;
  RQLParserStaticData& operator=(const RQLParserStaticData&) = delete;
  RQLParserStaticData& operator=(RQLParserStaticData&&) = delete;

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
      "prog", "default_statement", "compiler_option", "select_statement", 
      "declare_statement", "rule_statement", "dumppart", "systempart", "rational_se", 
      "retention_from", "fraction_rule", "field_declaration", "field_type", 
      "select_list", "field_id", "unary_op_expression", "asterisk", "expression", 
      "logic", "expression_logic", "term_logic", "expression_factor", "term", 
      "stream_expression", "stream_factor", "gen_index", "agregator", "stream_fn_call", 
      "window_agg", "function_call"
    },
    std::vector<std::string>{
      "", "'['", "']'", "'('", "')'", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "'='", 
      "'!='", "'>'", "'<'", "'>='", "'<='", "'!'", "'||'", "'.'", "'_'", 
      "'@'", "'#'", "'&'", "'%'", "'$'", "','", "';'", "':'", "'::'", "'*'", 
      "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", 
      "FLOAT_T", "DOUBLE_T", "SELECT", "STREAM", "FROM", "DECLARE", "RETENTION", 
      "FILE", "STORAGE", "ROTATION", "SUBSTRAT", "RULE", "DISPOSABLE", "ONESHOT", 
      "HOLD", "VOLATILE", "PERSISTENT", "DEFAULT", "ON", "WHEN", "DUMP", 
      "SYSTEM", "DO", "TO", "AND_C", "OR_C", "NOT_C", "MIN", "MAX", "AVG", 
      "SUMC", "TYPE_PROFILE", "STRING_PROFILE", "ID", "STRING", "FLOAT", 
      "DECIMAL", "REAL", "IS_EQ", "IS_NQ", "IS_GR", "IS_LS", "IS_GE", "IS_LE", 
      "EXCLAMATION", "DOUBLE_BAR", "DOT", "UNDERLINE", "AT", "SHARP", "AND", 
      "MOD", "DOLLAR", "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR", 
      "DIVIDE", "PLUS", "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", 
      "COMMENT", "LINE_COMMENT2"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,75,426,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,1,0,1,0,1,0,1,0,1,0,4,0,66,8,0,11,0,12,0,67,1,0,1,0,1,1,
  	1,1,1,1,1,2,1,2,1,2,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,85,8,3,1,3,1,3,1,
  	3,1,3,3,3,91,8,3,1,3,3,3,94,8,3,1,3,3,3,97,8,3,1,3,1,3,3,3,101,8,3,1,
  	4,1,4,1,4,1,4,5,4,107,8,4,10,4,12,4,110,9,4,1,4,1,4,1,4,1,4,1,4,1,4,1,
  	4,3,4,119,8,4,1,4,3,4,122,8,4,1,4,3,4,125,8,4,1,5,1,5,1,5,1,5,1,5,1,5,
  	1,5,1,5,1,5,3,5,136,8,5,1,6,1,6,3,6,140,8,6,1,6,1,6,1,6,3,6,145,8,6,1,
  	6,1,6,1,6,3,6,150,8,6,1,7,1,7,1,7,1,8,1,8,1,8,3,8,158,8,8,1,9,1,9,1,9,
  	3,9,163,8,9,1,10,1,10,1,10,1,10,1,11,1,11,1,11,1,11,1,11,3,11,174,8,11,
  	1,12,1,12,1,12,1,12,1,12,1,12,3,12,182,8,12,1,13,1,13,1,13,1,13,5,13,
  	188,8,13,10,13,12,13,191,9,13,3,13,193,8,13,1,14,1,14,1,14,1,14,1,14,
  	1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,14,3,14,212,
  	8,14,1,15,1,15,1,15,1,15,3,15,218,8,15,1,16,1,16,3,16,222,8,16,1,16,1,
  	16,1,17,1,17,1,18,1,18,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,5,
  	19,239,8,19,10,19,12,19,242,9,19,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,
  	20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,5,
  	20,265,8,20,10,20,12,20,268,9,20,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,
  	21,1,21,5,21,279,8,21,10,21,12,21,282,9,21,1,22,1,22,1,22,1,22,1,22,1,
  	22,3,22,290,8,22,1,22,1,22,3,22,294,8,22,1,22,1,22,1,22,1,22,1,22,1,22,
  	1,22,1,22,1,22,1,22,3,22,306,8,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,
  	1,22,1,22,5,22,317,8,22,10,22,12,22,320,9,22,1,23,1,23,1,23,1,23,1,23,
  	1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,3,23,337,8,23,1,23,
  	1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,
  	1,23,1,23,5,23,356,8,23,10,23,12,23,359,9,23,1,24,1,24,1,24,1,24,1,24,
  	1,24,1,24,1,24,1,24,1,24,1,24,3,24,372,8,24,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,3,25,381,8,25,1,25,1,25,1,25,1,25,1,25,1,25,5,25,389,8,25,10,
  	25,12,25,392,9,25,1,26,1,26,1,26,1,26,3,26,398,8,26,1,27,1,27,1,27,1,
  	27,1,27,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,29,1,29,1,29,1,29,1,29,1,
  	29,1,29,1,29,1,29,1,29,1,29,1,29,3,29,424,8,29,1,29,0,6,38,40,42,44,46,
  	50,30,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,
  	46,48,50,52,54,56,58,0,6,1,0,17,19,2,0,41,41,43,43,1,0,24,25,2,0,26,26,
  	40,40,1,0,68,69,1,0,36,39,475,0,65,1,0,0,0,2,71,1,0,0,0,4,74,1,0,0,0,
  	6,77,1,0,0,0,8,102,1,0,0,0,10,126,1,0,0,0,12,137,1,0,0,0,14,151,1,0,0,
  	0,16,157,1,0,0,0,18,159,1,0,0,0,20,164,1,0,0,0,22,168,1,0,0,0,24,181,
  	1,0,0,0,26,192,1,0,0,0,28,211,1,0,0,0,30,217,1,0,0,0,32,221,1,0,0,0,34,
  	225,1,0,0,0,36,227,1,0,0,0,38,229,1,0,0,0,40,243,1,0,0,0,42,269,1,0,0,
  	0,44,305,1,0,0,0,46,321,1,0,0,0,48,371,1,0,0,0,50,380,1,0,0,0,52,397,
  	1,0,0,0,54,399,1,0,0,0,56,404,1,0,0,0,58,423,1,0,0,0,60,66,3,6,3,0,61,
  	66,3,8,4,0,62,66,3,4,2,0,63,66,3,2,1,0,64,66,3,10,5,0,65,60,1,0,0,0,65,
  	61,1,0,0,0,65,62,1,0,0,0,65,63,1,0,0,0,65,64,1,0,0,0,66,67,1,0,0,0,67,
  	65,1,0,0,0,67,68,1,0,0,0,68,69,1,0,0,0,69,70,5,0,0,1,70,1,1,0,0,0,71,
  	72,5,26,0,0,72,73,5,24,0,0,73,3,1,0,0,0,74,75,7,0,0,0,75,76,7,1,0,0,76,
  	5,1,0,0,0,77,78,5,11,0,0,78,79,3,26,13,0,79,80,5,12,0,0,80,84,5,42,0,
  	0,81,82,5,1,0,0,82,83,5,45,0,0,83,85,5,2,0,0,84,81,1,0,0,0,84,85,1,0,
  	0,0,85,86,1,0,0,0,86,87,5,13,0,0,87,90,3,46,23,0,88,89,5,16,0,0,89,91,
  	5,43,0,0,90,88,1,0,0,0,90,91,1,0,0,0,91,93,1,0,0,0,92,94,3,18,9,0,93,
  	92,1,0,0,0,93,94,1,0,0,0,94,96,1,0,0,0,95,97,7,2,0,0,96,95,1,0,0,0,96,
  	97,1,0,0,0,97,100,1,0,0,0,98,99,5,17,0,0,99,101,7,3,0,0,100,98,1,0,0,
  	0,100,101,1,0,0,0,101,7,1,0,0,0,102,103,5,14,0,0,103,108,3,22,11,0,104,
  	105,5,62,0,0,105,107,3,22,11,0,106,104,1,0,0,0,107,110,1,0,0,0,108,106,
  	1,0,0,0,108,109,1,0,0,0,109,111,1,0,0,0,110,108,1,0,0,0,111,112,5,12,
  	0,0,112,113,5,42,0,0,113,114,5,62,0,0,114,115,3,16,8,0,115,116,5,16,0,
  	0,116,118,5,43,0,0,117,119,5,21,0,0,118,117,1,0,0,0,118,119,1,0,0,0,119,
  	121,1,0,0,0,120,122,5,22,0,0,121,120,1,0,0,0,121,122,1,0,0,0,122,124,
  	1,0,0,0,123,125,5,23,0,0,124,123,1,0,0,0,124,125,1,0,0,0,125,9,1,0,0,
  	0,126,127,5,20,0,0,127,128,5,42,0,0,128,129,5,27,0,0,129,130,5,42,0,0,
  	130,131,5,28,0,0,131,132,3,36,18,0,132,135,5,31,0,0,133,136,3,12,6,0,
  	134,136,3,14,7,0,135,133,1,0,0,0,135,134,1,0,0,0,136,11,1,0,0,0,137,139,
  	5,29,0,0,138,140,5,69,0,0,139,138,1,0,0,0,139,140,1,0,0,0,140,141,1,0,
  	0,0,141,142,5,45,0,0,142,144,5,32,0,0,143,145,5,69,0,0,144,143,1,0,0,
  	0,144,145,1,0,0,0,145,146,1,0,0,0,146,149,5,45,0,0,147,148,5,15,0,0,148,
  	150,5,45,0,0,149,147,1,0,0,0,149,150,1,0,0,0,150,13,1,0,0,0,151,152,5,
  	30,0,0,152,153,5,43,0,0,153,15,1,0,0,0,154,158,3,20,10,0,155,158,5,44,
  	0,0,156,158,5,45,0,0,157,154,1,0,0,0,157,155,1,0,0,0,157,156,1,0,0,0,
  	158,17,1,0,0,0,159,160,5,15,0,0,160,162,5,45,0,0,161,163,5,45,0,0,162,
  	161,1,0,0,0,162,163,1,0,0,0,163,19,1,0,0,0,164,165,5,45,0,0,165,166,5,
  	67,0,0,166,167,5,45,0,0,167,21,1,0,0,0,168,169,5,42,0,0,169,173,3,24,
  	12,0,170,171,5,1,0,0,171,172,5,45,0,0,172,174,5,2,0,0,173,170,1,0,0,0,
  	173,174,1,0,0,0,174,23,1,0,0,0,175,182,5,5,0,0,176,182,5,8,0,0,177,182,
  	5,7,0,0,178,182,5,9,0,0,179,182,5,10,0,0,180,182,5,6,0,0,181,175,1,0,
  	0,0,181,176,1,0,0,0,181,177,1,0,0,0,181,178,1,0,0,0,181,179,1,0,0,0,181,
  	180,1,0,0,0,182,25,1,0,0,0,183,193,3,32,16,0,184,189,3,34,17,0,185,186,
  	5,62,0,0,186,188,3,34,17,0,187,185,1,0,0,0,188,191,1,0,0,0,189,187,1,
  	0,0,0,189,190,1,0,0,0,190,193,1,0,0,0,191,189,1,0,0,0,192,183,1,0,0,0,
  	192,184,1,0,0,0,193,27,1,0,0,0,194,212,5,42,0,0,195,196,5,42,0,0,196,
  	197,5,1,0,0,197,198,5,56,0,0,198,212,5,2,0,0,199,200,5,42,0,0,200,201,
  	5,55,0,0,201,212,5,42,0,0,202,203,5,42,0,0,203,204,5,1,0,0,204,205,5,
  	45,0,0,205,212,5,2,0,0,206,207,5,42,0,0,207,208,5,1,0,0,208,209,3,50,
  	25,0,209,210,5,2,0,0,210,212,1,0,0,0,211,194,1,0,0,0,211,195,1,0,0,0,
  	211,199,1,0,0,0,211,202,1,0,0,0,211,206,1,0,0,0,212,29,1,0,0,0,213,214,
  	5,70,0,0,214,218,3,34,17,0,215,216,7,4,0,0,216,218,3,34,17,0,217,213,
  	1,0,0,0,217,215,1,0,0,0,218,31,1,0,0,0,219,220,5,42,0,0,220,222,5,55,
  	0,0,221,219,1,0,0,0,221,222,1,0,0,0,222,223,1,0,0,0,223,224,5,66,0,0,
  	224,33,1,0,0,0,225,226,3,42,21,0,226,35,1,0,0,0,227,228,3,38,19,0,228,
  	37,1,0,0,0,229,230,6,19,-1,0,230,231,3,40,20,0,231,240,1,0,0,0,232,233,
  	10,3,0,0,233,234,5,33,0,0,234,239,3,38,19,4,235,236,10,2,0,0,236,237,
  	5,34,0,0,237,239,3,38,19,3,238,232,1,0,0,0,238,235,1,0,0,0,239,242,1,
  	0,0,0,240,238,1,0,0,0,240,241,1,0,0,0,241,39,1,0,0,0,242,240,1,0,0,0,
  	243,244,6,20,-1,0,244,245,3,42,21,0,245,266,1,0,0,0,246,247,10,7,0,0,
  	247,248,5,47,0,0,248,265,3,40,20,8,249,250,10,6,0,0,250,251,5,48,0,0,
  	251,265,3,40,20,7,252,253,10,5,0,0,253,254,5,49,0,0,254,265,3,40,20,6,
  	255,256,10,4,0,0,256,257,5,50,0,0,257,265,3,40,20,5,258,259,10,3,0,0,
  	259,260,5,51,0,0,260,265,3,40,20,4,261,262,10,2,0,0,262,263,5,52,0,0,
  	263,265,3,40,20,3,264,246,1,0,0,0,264,249,1,0,0,0,264,252,1,0,0,0,264,
  	255,1,0,0,0,264,258,1,0,0,0,264,261,1,0,0,0,265,268,1,0,0,0,266,264,1,
  	0,0,0,266,267,1,0,0,0,267,41,1,0,0,0,268,266,1,0,0,0,269,270,6,21,-1,
  	0,270,271,3,44,22,0,271,280,1,0,0,0,272,273,10,3,0,0,273,274,5,68,0,0,
  	274,279,3,42,21,4,275,276,10,2,0,0,276,277,5,69,0,0,277,279,3,42,21,3,
  	278,272,1,0,0,0,278,275,1,0,0,0,279,282,1,0,0,0,280,278,1,0,0,0,280,281,
  	1,0,0,0,281,43,1,0,0,0,282,280,1,0,0,0,283,284,6,22,-1,0,284,285,5,3,
  	0,0,285,286,3,42,21,0,286,287,5,4,0,0,287,306,1,0,0,0,288,290,5,69,0,
  	0,289,288,1,0,0,0,289,290,1,0,0,0,290,291,1,0,0,0,291,306,5,44,0,0,292,
  	294,5,69,0,0,293,292,1,0,0,0,293,294,1,0,0,0,294,295,1,0,0,0,295,306,
  	5,45,0,0,296,306,5,43,0,0,297,306,3,30,15,0,298,306,3,28,14,0,299,306,
  	3,56,28,0,300,306,3,52,26,0,301,306,5,61,0,0,302,306,3,58,29,0,303,304,
  	5,35,0,0,304,306,3,44,22,1,305,283,1,0,0,0,305,289,1,0,0,0,305,293,1,
  	0,0,0,305,296,1,0,0,0,305,297,1,0,0,0,305,298,1,0,0,0,305,299,1,0,0,0,
  	305,300,1,0,0,0,305,301,1,0,0,0,305,302,1,0,0,0,305,303,1,0,0,0,306,318,
  	1,0,0,0,307,308,10,14,0,0,308,309,5,72,0,0,309,317,3,44,22,14,310,311,
  	10,13,0,0,311,312,5,66,0,0,312,317,3,44,22,14,313,314,10,12,0,0,314,315,
  	5,67,0,0,315,317,3,44,22,13,316,307,1,0,0,0,316,310,1,0,0,0,316,313,1,
  	0,0,0,317,320,1,0,0,0,318,316,1,0,0,0,318,319,1,0,0,0,319,45,1,0,0,0,
  	320,318,1,0,0,0,321,322,6,23,-1,0,322,323,3,48,24,0,323,357,1,0,0,0,324,
  	325,10,3,0,0,325,326,5,58,0,0,326,356,3,46,23,4,327,328,10,2,0,0,328,
  	329,5,68,0,0,329,356,3,46,23,3,330,331,10,9,0,0,331,332,5,57,0,0,332,
  	333,5,3,0,0,333,334,5,45,0,0,334,336,5,62,0,0,335,337,5,69,0,0,336,335,
  	1,0,0,0,336,337,1,0,0,0,337,338,1,0,0,0,338,339,5,45,0,0,339,356,5,4,
  	0,0,340,341,10,8,0,0,341,342,5,59,0,0,342,356,3,16,8,0,343,344,10,7,0,
  	0,344,345,5,60,0,0,345,356,3,16,8,0,346,347,10,6,0,0,347,348,5,49,0,0,
  	348,356,5,45,0,0,349,350,10,5,0,0,350,351,5,69,0,0,351,356,3,16,8,0,352,
  	353,10,4,0,0,353,354,5,55,0,0,354,356,3,52,26,0,355,324,1,0,0,0,355,327,
  	1,0,0,0,355,330,1,0,0,0,355,340,1,0,0,0,355,343,1,0,0,0,355,346,1,0,0,
  	0,355,349,1,0,0,0,355,352,1,0,0,0,356,359,1,0,0,0,357,355,1,0,0,0,357,
  	358,1,0,0,0,358,47,1,0,0,0,359,357,1,0,0,0,360,372,5,42,0,0,361,362,5,
  	42,0,0,362,363,5,1,0,0,363,364,3,50,25,0,364,365,5,2,0,0,365,372,1,0,
  	0,0,366,367,5,3,0,0,367,368,3,46,23,0,368,369,5,4,0,0,369,372,1,0,0,0,
  	370,372,3,54,27,0,371,360,1,0,0,0,371,361,1,0,0,0,371,366,1,0,0,0,371,
  	370,1,0,0,0,372,49,1,0,0,0,373,374,6,25,-1,0,374,375,5,3,0,0,375,376,
  	3,50,25,0,376,377,5,4,0,0,377,381,1,0,0,0,378,381,5,61,0,0,379,381,5,
  	45,0,0,380,373,1,0,0,0,380,378,1,0,0,0,380,379,1,0,0,0,381,390,1,0,0,
  	0,382,383,10,5,0,0,383,384,5,66,0,0,384,389,3,50,25,6,385,386,10,4,0,
  	0,386,387,7,4,0,0,387,389,3,50,25,5,388,382,1,0,0,0,388,385,1,0,0,0,389,
  	392,1,0,0,0,390,388,1,0,0,0,390,391,1,0,0,0,391,51,1,0,0,0,392,390,1,
  	0,0,0,393,398,5,36,0,0,394,398,5,37,0,0,395,398,5,38,0,0,396,398,5,39,
  	0,0,397,393,1,0,0,0,397,394,1,0,0,0,397,395,1,0,0,0,397,396,1,0,0,0,398,
  	53,1,0,0,0,399,400,7,5,0,0,400,401,5,3,0,0,401,402,3,46,23,0,402,403,
  	5,4,0,0,403,55,1,0,0,0,404,405,7,5,0,0,405,406,5,3,0,0,406,407,3,42,21,
  	0,407,408,5,64,0,0,408,409,5,45,0,0,409,410,5,4,0,0,410,57,1,0,0,0,411,
  	412,5,42,0,0,412,413,5,3,0,0,413,414,3,42,21,0,414,415,5,4,0,0,415,424,
  	1,0,0,0,416,417,5,42,0,0,417,418,5,3,0,0,418,419,3,42,21,0,419,420,5,
  	64,0,0,420,421,5,45,0,0,421,422,5,4,0,0,422,424,1,0,0,0,423,411,1,0,0,
  	0,423,416,1,0,0,0,424,59,1,0,0,0,44,65,67,84,90,93,96,100,108,118,121,
  	124,135,139,144,149,157,162,173,181,189,192,211,217,221,238,240,264,266,
  	278,280,289,293,305,316,318,336,355,357,371,380,388,390,397,423
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  rqlParserStaticData = staticData.release();
}

}

RQLParser::RQLParser(TokenStream *input) : RQLParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

RQLParser::RQLParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  RQLParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *rqlParserStaticData->atn, rqlParserStaticData->decisionToDFA, rqlParserStaticData->sharedContextCache, options);
}

RQLParser::~RQLParser() {
  delete _interpreter;
}

const atn::ATN& RQLParser::getATN() const {
  return *rqlParserStaticData->atn;
}

std::string RQLParser::getGrammarFileName() const {
  return "RQL.g4";
}

const std::vector<std::string>& RQLParser::getRuleNames() const {
  return rqlParserStaticData->ruleNames;
}

const dfa::Vocabulary& RQLParser::getVocabulary() const {
  return rqlParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView RQLParser::getSerializedATN() const {
  return rqlParserStaticData->serializedATN;
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

std::vector<RQLParser::Compiler_optionContext *> RQLParser::ProgContext::compiler_option() {
  return getRuleContexts<RQLParser::Compiler_optionContext>();
}

RQLParser::Compiler_optionContext* RQLParser::ProgContext::compiler_option(size_t i) {
  return getRuleContext<RQLParser::Compiler_optionContext>(i);
}

std::vector<RQLParser::Default_statementContext *> RQLParser::ProgContext::default_statement() {
  return getRuleContexts<RQLParser::Default_statementContext>();
}

RQLParser::Default_statementContext* RQLParser::ProgContext::default_statement(size_t i) {
  return getRuleContext<RQLParser::Default_statementContext>(i);
}

std::vector<RQLParser::Rule_statementContext *> RQLParser::ProgContext::rule_statement() {
  return getRuleContexts<RQLParser::Rule_statementContext>();
}

RQLParser::Rule_statementContext* RQLParser::ProgContext::rule_statement(size_t i) {
  return getRuleContext<RQLParser::Rule_statementContext>(i);
}


size_t RQLParser::ProgContext::getRuleIndex() const {
  return RQLParser::RuleProg;
}

void RQLParser::ProgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProg(this);
}

void RQLParser::ProgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProg(this);
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
    setState(65); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(65);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(60);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(61);
          declare_statement();
          break;
        }

        case RQLParser::STORAGE:
        case RQLParser::ROTATION:
        case RQLParser::SUBSTRAT: {
          setState(62);
          compiler_option();
          break;
        }

        case RQLParser::DEFAULT: {
          setState(63);
          default_statement();
          break;
        }

        case RQLParser::RULE: {
          setState(64);
          rule_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(67); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 69093376) != 0));
    setState(69);
    match(RQLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Default_statementContext ------------------------------------------------------------------

RQLParser::Default_statementContext::Default_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Default_statementContext::getRuleIndex() const {
  return RQLParser::RuleDefault_statement;
}

void RQLParser::Default_statementContext::copyFrom(Default_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- DefaultOptionContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::DefaultOptionContext::DEFAULT() {
  return getToken(RQLParser::DEFAULT, 0);
}

tree::TerminalNode* RQLParser::DefaultOptionContext::VOLATILE() {
  return getToken(RQLParser::VOLATILE, 0);
}

RQLParser::DefaultOptionContext::DefaultOptionContext(Default_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::DefaultOptionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDefaultOption(this);
}
void RQLParser::DefaultOptionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDefaultOption(this);
}
RQLParser::Default_statementContext* RQLParser::default_statement() {
  Default_statementContext *_localctx = _tracker.createInstance<Default_statementContext>(_ctx, getState());
  enterRule(_localctx, 2, RQLParser::RuleDefault_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::DefaultOptionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(71);
    match(RQLParser::DEFAULT);
    setState(72);
    match(RQLParser::VOLATILE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Compiler_optionContext ------------------------------------------------------------------

RQLParser::Compiler_optionContext::Compiler_optionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Compiler_optionContext::getRuleIndex() const {
  return RQLParser::RuleCompiler_option;
}

void RQLParser::Compiler_optionContext::copyFrom(Compiler_optionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- CoptionContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::CoptionContext::ROTATION() {
  return getToken(RQLParser::ROTATION, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::STORAGE() {
  return getToken(RQLParser::STORAGE, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::SUBSTRAT() {
  return getToken(RQLParser::SUBSTRAT, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::STRING_PROFILE() {
  return getToken(RQLParser::STRING_PROFILE, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

RQLParser::CoptionContext::CoptionContext(Compiler_optionContext *ctx) { copyFrom(ctx); }

void RQLParser::CoptionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCoption(this);
}
void RQLParser::CoptionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCoption(this);
}
RQLParser::Compiler_optionContext* RQLParser::compiler_option() {
  Compiler_optionContext *_localctx = _tracker.createInstance<Compiler_optionContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleCompiler_option);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::CoptionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(74);
    antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 917504) != 0))) {
      antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(75);
    antlrcpp::downCast<CoptionContext *>(_localctx)->value = _input->LT(1);
    _la = _input->LA(1);
    if (!(_la == RQLParser::STRING_PROFILE

    || _la == RQLParser::STRING)) {
      antlrcpp::downCast<CoptionContext *>(_localctx)->value = _errHandler->recoverInline(this);
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

//----------------- Select_statementContext ------------------------------------------------------------------

RQLParser::Select_statementContext::Select_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Select_statementContext::getRuleIndex() const {
  return RQLParser::RuleSelect_statement;
}

void RQLParser::Select_statementContext::copyFrom(Select_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SelectContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::SelectContext::SELECT() {
  return getToken(RQLParser::SELECT, 0);
}

RQLParser::Select_listContext* RQLParser::SelectContext::select_list() {
  return getRuleContext<RQLParser::Select_listContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

tree::TerminalNode* RQLParser::SelectContext::FROM() {
  return getToken(RQLParser::FROM, 0);
}

RQLParser::Stream_expressionContext* RQLParser::SelectContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::SelectContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

RQLParser::Retention_fromContext* RQLParser::SelectContext::retention_from() {
  return getRuleContext<RQLParser::Retention_fromContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::STORAGE() {
  return getToken(RQLParser::STORAGE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::SelectContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

tree::TerminalNode* RQLParser::SelectContext::VOLATILE() {
  return getToken(RQLParser::VOLATILE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::PERSISTENT() {
  return getToken(RQLParser::PERSISTENT, 0);
}

tree::TerminalNode* RQLParser::SelectContext::TYPE_PROFILE() {
  return getToken(RQLParser::TYPE_PROFILE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::DEFAULT() {
  return getToken(RQLParser::DEFAULT, 0);
}

RQLParser::SelectContext::SelectContext(Select_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelect(this);
}
void RQLParser::SelectContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelect(this);
}
RQLParser::Select_statementContext* RQLParser::select_statement() {
  Select_statementContext *_localctx = _tracker.createInstance<Select_statementContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleSelect_statement);
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
    setState(77);
    match(RQLParser::SELECT);
    setState(78);
    select_list();
    setState(79);
    match(RQLParser::STREAM);
    setState(80);
    antlrcpp::downCast<SelectContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(84);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(81);
      match(RQLParser::T__0);
      setState(82);
      antlrcpp::downCast<SelectContext *>(_localctx)->gen_size = match(RQLParser::DECIMAL);
      setState(83);
      match(RQLParser::T__1);
    }
    setState(86);
    match(RQLParser::FROM);
    setState(87);
    stream_expression(0);
    setState(90);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(88);
      match(RQLParser::FILE);
      setState(89);
      antlrcpp::downCast<SelectContext *>(_localctx)->file_name = match(RQLParser::STRING);
    }
    setState(93);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(92);
      retention_from();
    }
    setState(96);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::VOLATILE

    || _la == RQLParser::PERSISTENT) {
      setState(95);
      _la = _input->LA(1);
      if (!(_la == RQLParser::VOLATILE

      || _la == RQLParser::PERSISTENT)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
    }
    setState(100);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx)) {
    case 1: {
      setState(98);
      match(RQLParser::STORAGE);
      setState(99);
      antlrcpp::downCast<SelectContext *>(_localctx)->type_name = _input->LT(1);
      _la = _input->LA(1);
      if (!(_la == RQLParser::DEFAULT

      || _la == RQLParser::TYPE_PROFILE)) {
        antlrcpp::downCast<SelectContext *>(_localctx)->type_name = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      break;
    }

    default:
      break;
    }
   
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


size_t RQLParser::Declare_statementContext::getRuleIndex() const {
  return RQLParser::RuleDeclare_statement;
}

void RQLParser::Declare_statementContext::copyFrom(Declare_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- DeclareContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::DeclareContext::DECLARE() {
  return getToken(RQLParser::DECLARE, 0);
}

std::vector<RQLParser::Field_declarationContext *> RQLParser::DeclareContext::field_declaration() {
  return getRuleContexts<RQLParser::Field_declarationContext>();
}

RQLParser::Field_declarationContext* RQLParser::DeclareContext::field_declaration(size_t i) {
  return getRuleContext<RQLParser::Field_declarationContext>(i);
}

tree::TerminalNode* RQLParser::DeclareContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

std::vector<tree::TerminalNode *> RQLParser::DeclareContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::DeclareContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

RQLParser::Rational_seContext* RQLParser::DeclareContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

tree::TerminalNode* RQLParser::DeclareContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::DISPOSABLE() {
  return getToken(RQLParser::DISPOSABLE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::ONESHOT() {
  return getToken(RQLParser::ONESHOT, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::HOLD() {
  return getToken(RQLParser::HOLD, 0);
}

RQLParser::DeclareContext::DeclareContext(Declare_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::DeclareContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDeclare(this);
}
void RQLParser::DeclareContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDeclare(this);
}
RQLParser::Declare_statementContext* RQLParser::declare_statement() {
  Declare_statementContext *_localctx = _tracker.createInstance<Declare_statementContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleDeclare_statement);
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
    setState(102);
    match(RQLParser::DECLARE);
    setState(103);
    field_declaration();
    setState(108);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(104);
      match(RQLParser::COMMA);
      setState(105);
      field_declaration();
      setState(110);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(111);
    match(RQLParser::STREAM);
    setState(112);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(113);
    match(RQLParser::COMMA);
    setState(114);
    rational_se();
    setState(115);
    match(RQLParser::FILE);
    setState(116);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);
    setState(118);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DISPOSABLE) {
      setState(117);
      match(RQLParser::DISPOSABLE);
    }
    setState(121);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ONESHOT) {
      setState(120);
      match(RQLParser::ONESHOT);
    }
    setState(124);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::HOLD) {
      setState(123);
      match(RQLParser::HOLD);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Rule_statementContext ------------------------------------------------------------------

RQLParser::Rule_statementContext::Rule_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Rule_statementContext::getRuleIndex() const {
  return RQLParser::RuleRule_statement;
}

void RQLParser::Rule_statementContext::copyFrom(Rule_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RulezContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RulezContext::RULE() {
  return getToken(RQLParser::RULE, 0);
}

tree::TerminalNode* RQLParser::RulezContext::ON() {
  return getToken(RQLParser::ON, 0);
}

tree::TerminalNode* RQLParser::RulezContext::WHEN() {
  return getToken(RQLParser::WHEN, 0);
}

RQLParser::LogicContext* RQLParser::RulezContext::logic() {
  return getRuleContext<RQLParser::LogicContext>(0);
}

tree::TerminalNode* RQLParser::RulezContext::DO() {
  return getToken(RQLParser::DO, 0);
}

std::vector<tree::TerminalNode *> RQLParser::RulezContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::RulezContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

RQLParser::DumppartContext* RQLParser::RulezContext::dumppart() {
  return getRuleContext<RQLParser::DumppartContext>(0);
}

RQLParser::SystempartContext* RQLParser::RulezContext::systempart() {
  return getRuleContext<RQLParser::SystempartContext>(0);
}

RQLParser::RulezContext::RulezContext(Rule_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::RulezContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRulez(this);
}
void RQLParser::RulezContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRulez(this);
}
RQLParser::Rule_statementContext* RQLParser::rule_statement() {
  Rule_statementContext *_localctx = _tracker.createInstance<Rule_statementContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleRule_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::RulezContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(126);
    match(RQLParser::RULE);
    setState(127);
    antlrcpp::downCast<RulezContext *>(_localctx)->name = match(RQLParser::ID);
    setState(128);
    match(RQLParser::ON);
    setState(129);
    antlrcpp::downCast<RulezContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(130);
    match(RQLParser::WHEN);
    setState(131);
    logic();
    setState(132);
    match(RQLParser::DO);
    setState(135);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::DUMP: {
        setState(133);
        dumppart();
        break;
      }

      case RQLParser::SYSTEM: {
        setState(134);
        systempart();
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

//----------------- DumppartContext ------------------------------------------------------------------

RQLParser::DumppartContext::DumppartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::DumppartContext::DUMP() {
  return getToken(RQLParser::DUMP, 0);
}

tree::TerminalNode* RQLParser::DumppartContext::TO() {
  return getToken(RQLParser::TO, 0);
}

std::vector<tree::TerminalNode *> RQLParser::DumppartContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::DumppartContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::DumppartContext::MINUS() {
  return getTokens(RQLParser::MINUS);
}

tree::TerminalNode* RQLParser::DumppartContext::MINUS(size_t i) {
  return getToken(RQLParser::MINUS, i);
}

tree::TerminalNode* RQLParser::DumppartContext::RETENTION() {
  return getToken(RQLParser::RETENTION, 0);
}


size_t RQLParser::DumppartContext::getRuleIndex() const {
  return RQLParser::RuleDumppart;
}

void RQLParser::DumppartContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDumppart(this);
}

void RQLParser::DumppartContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDumppart(this);
}

RQLParser::DumppartContext* RQLParser::dumppart() {
  DumppartContext *_localctx = _tracker.createInstance<DumppartContext>(_ctx, getState());
  enterRule(_localctx, 12, RQLParser::RuleDumppart);
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
    setState(137);
    match(RQLParser::DUMP);
    setState(139);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(138);
      match(RQLParser::MINUS);
    }
    setState(141);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_back = match(RQLParser::DECIMAL);
    setState(142);
    match(RQLParser::TO);
    setState(144);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(143);
      match(RQLParser::MINUS);
    }
    setState(146);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_forward = match(RQLParser::DECIMAL);
    setState(149);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(147);
      match(RQLParser::RETENTION);
      setState(148);
      antlrcpp::downCast<DumppartContext *>(_localctx)->rule_retnetion = match(RQLParser::DECIMAL);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SystempartContext ------------------------------------------------------------------

RQLParser::SystempartContext::SystempartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::SystempartContext::SYSTEM() {
  return getToken(RQLParser::SYSTEM, 0);
}

tree::TerminalNode* RQLParser::SystempartContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}


size_t RQLParser::SystempartContext::getRuleIndex() const {
  return RQLParser::RuleSystempart;
}

void RQLParser::SystempartContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSystempart(this);
}

void RQLParser::SystempartContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSystempart(this);
}

RQLParser::SystempartContext* RQLParser::systempart() {
  SystempartContext *_localctx = _tracker.createInstance<SystempartContext>(_ctx, getState());
  enterRule(_localctx, 14, RQLParser::RuleSystempart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(151);
    match(RQLParser::SYSTEM);
    setState(152);
    antlrcpp::downCast<SystempartContext *>(_localctx)->syscmd = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Rational_seContext ------------------------------------------------------------------

RQLParser::Rational_seContext::Rational_seContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Rational_seContext::getRuleIndex() const {
  return RQLParser::RuleRational_se;
}

void RQLParser::Rational_seContext::copyFrom(Rational_seContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RationalAsDecimalContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RationalAsDecimalContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::RationalAsDecimalContext::RationalAsDecimalContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsDecimalContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsDecimal(this);
}
void RQLParser::RationalAsDecimalContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsDecimal(this);
}
//----------------- RationalAsFloatContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RationalAsFloatContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

RQLParser::RationalAsFloatContext::RationalAsFloatContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsFloat(this);
}
void RQLParser::RationalAsFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsFloat(this);
}
//----------------- RationalAsFraction_proformaContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext* RQLParser::RationalAsFraction_proformaContext::fraction_rule() {
  return getRuleContext<RQLParser::Fraction_ruleContext>(0);
}

RQLParser::RationalAsFraction_proformaContext::RationalAsFraction_proformaContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsFraction_proformaContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsFraction_proforma(this);
}
void RQLParser::RationalAsFraction_proformaContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsFraction_proforma(this);
}
RQLParser::Rational_seContext* RQLParser::rational_se() {
  Rational_seContext *_localctx = _tracker.createInstance<Rational_seContext>(_ctx, getState());
  enterRule(_localctx, 16, RQLParser::RuleRational_se);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(157);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFraction_proformaContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(154);
      fraction_rule();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFloatContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(155);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsDecimalContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(156);
      match(RQLParser::DECIMAL);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Retention_fromContext ------------------------------------------------------------------

RQLParser::Retention_fromContext::Retention_fromContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Retention_fromContext::getRuleIndex() const {
  return RQLParser::RuleRetention_from;
}

void RQLParser::Retention_fromContext::copyFrom(Retention_fromContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RetentionContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RetentionContext::RETENTION() {
  return getToken(RQLParser::RETENTION, 0);
}

std::vector<tree::TerminalNode *> RQLParser::RetentionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::RetentionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

RQLParser::RetentionContext::RetentionContext(Retention_fromContext *ctx) { copyFrom(ctx); }

void RQLParser::RetentionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRetention(this);
}
void RQLParser::RetentionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRetention(this);
}
RQLParser::Retention_fromContext* RQLParser::retention_from() {
  Retention_fromContext *_localctx = _tracker.createInstance<Retention_fromContext>(_ctx, getState());
  enterRule(_localctx, 18, RQLParser::RuleRetention_from);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::RetentionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(159);
    match(RQLParser::RETENTION);
    setState(160);
    antlrcpp::downCast<RetentionContext *>(_localctx)->capacity = match(RQLParser::DECIMAL);
    setState(162);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DECIMAL) {
      setState(161);
      antlrcpp::downCast<RetentionContext *>(_localctx)->segments = match(RQLParser::DECIMAL);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Fraction_ruleContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext::Fraction_ruleContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Fraction_ruleContext::getRuleIndex() const {
  return RQLParser::RuleFraction_rule;
}

void RQLParser::Fraction_ruleContext::copyFrom(Fraction_ruleContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FractionContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> RQLParser::FractionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::FractionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

tree::TerminalNode* RQLParser::FractionContext::DIVIDE() {
  return getToken(RQLParser::DIVIDE, 0);
}

RQLParser::FractionContext::FractionContext(Fraction_ruleContext *ctx) { copyFrom(ctx); }

void RQLParser::FractionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFraction(this);
}
void RQLParser::FractionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFraction(this);
}
RQLParser::Fraction_ruleContext* RQLParser::fraction_rule() {
  Fraction_ruleContext *_localctx = _tracker.createInstance<Fraction_ruleContext>(_ctx, getState());
  enterRule(_localctx, 20, RQLParser::RuleFraction_rule);

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
    setState(164);
    match(RQLParser::DECIMAL);
    setState(165);
    match(RQLParser::DIVIDE);
    setState(166);
    match(RQLParser::DECIMAL);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_declarationContext ------------------------------------------------------------------

RQLParser::Field_declarationContext::Field_declarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Field_declarationContext::getRuleIndex() const {
  return RQLParser::RuleField_declaration;
}

void RQLParser::Field_declarationContext::copyFrom(Field_declarationContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SingleDeclarationContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::SingleDeclarationContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::Field_typeContext* RQLParser::SingleDeclarationContext::field_type() {
  return getRuleContext<RQLParser::Field_typeContext>(0);
}

tree::TerminalNode* RQLParser::SingleDeclarationContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::SingleDeclarationContext::SingleDeclarationContext(Field_declarationContext *ctx) { copyFrom(ctx); }

void RQLParser::SingleDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSingleDeclaration(this);
}
void RQLParser::SingleDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSingleDeclaration(this);
}
RQLParser::Field_declarationContext* RQLParser::field_declaration() {
  Field_declarationContext *_localctx = _tracker.createInstance<Field_declarationContext>(_ctx, getState());
  enterRule(_localctx, 22, RQLParser::RuleField_declaration);
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
    setState(168);
    match(RQLParser::ID);
    setState(169);
    field_type();
    setState(173);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(170);
      match(RQLParser::T__0);
      setState(171);
      antlrcpp::downCast<SingleDeclarationContext *>(_localctx)->type_size = match(RQLParser::DECIMAL);
      setState(172);
      match(RQLParser::T__1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_typeContext ------------------------------------------------------------------

RQLParser::Field_typeContext::Field_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Field_typeContext::getRuleIndex() const {
  return RQLParser::RuleField_type;
}

void RQLParser::Field_typeContext::copyFrom(Field_typeContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- TypeUnsignedContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeUnsignedContext::UNSIGNED_T() {
  return getToken(RQLParser::UNSIGNED_T, 0);
}

RQLParser::TypeUnsignedContext::TypeUnsignedContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeUnsignedContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeUnsigned(this);
}
void RQLParser::TypeUnsignedContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeUnsigned(this);
}
//----------------- TypeIntContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeIntContext::INTEGER_T() {
  return getToken(RQLParser::INTEGER_T, 0);
}

RQLParser::TypeIntContext::TypeIntContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeIntContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeInt(this);
}
void RQLParser::TypeIntContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeInt(this);
}
//----------------- TypeFloatContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeFloatContext::FLOAT_T() {
  return getToken(RQLParser::FLOAT_T, 0);
}

RQLParser::TypeFloatContext::TypeFloatContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeFloat(this);
}
void RQLParser::TypeFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeFloat(this);
}
//----------------- TypeStringContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeStringContext::STRING_T() {
  return getToken(RQLParser::STRING_T, 0);
}

RQLParser::TypeStringContext::TypeStringContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeString(this);
}
void RQLParser::TypeStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeString(this);
}
//----------------- TypeByteContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeByteContext::BYTE_T() {
  return getToken(RQLParser::BYTE_T, 0);
}

RQLParser::TypeByteContext::TypeByteContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeByteContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeByte(this);
}
void RQLParser::TypeByteContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeByte(this);
}
//----------------- TypeDoubleContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeDoubleContext::DOUBLE_T() {
  return getToken(RQLParser::DOUBLE_T, 0);
}

RQLParser::TypeDoubleContext::TypeDoubleContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeDoubleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeDouble(this);
}
void RQLParser::TypeDoubleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeDouble(this);
}
RQLParser::Field_typeContext* RQLParser::field_type() {
  Field_typeContext *_localctx = _tracker.createInstance<Field_typeContext>(_ctx, getState());
  enterRule(_localctx, 24, RQLParser::RuleField_type);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(181);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BYTE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(175);
        match(RQLParser::BYTE_T);
        break;
      }

      case RQLParser::INTEGER_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeIntContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(176);
        match(RQLParser::INTEGER_T);
        break;
      }

      case RQLParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeUnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(177);
        match(RQLParser::UNSIGNED_T);
        break;
      }

      case RQLParser::FLOAT_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeFloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(178);
        match(RQLParser::FLOAT_T);
        break;
      }

      case RQLParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeDoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(179);
        match(RQLParser::DOUBLE_T);
        break;
      }

      case RQLParser::STRING_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeStringContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(180);
        match(RQLParser::STRING_T);
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

//----------------- Select_listContext ------------------------------------------------------------------

RQLParser::Select_listContext::Select_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Select_listContext::getRuleIndex() const {
  return RQLParser::RuleSelect_list;
}

void RQLParser::Select_listContext::copyFrom(Select_listContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SelectListContext ------------------------------------------------------------------

std::vector<RQLParser::ExpressionContext *> RQLParser::SelectListContext::expression() {
  return getRuleContexts<RQLParser::ExpressionContext>();
}

RQLParser::ExpressionContext* RQLParser::SelectListContext::expression(size_t i) {
  return getRuleContext<RQLParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::SelectListContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::SelectListContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

RQLParser::SelectListContext::SelectListContext(Select_listContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelectList(this);
}
void RQLParser::SelectListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelectList(this);
}
//----------------- SelectListFullscanContext ------------------------------------------------------------------

RQLParser::AsteriskContext* RQLParser::SelectListFullscanContext::asterisk() {
  return getRuleContext<RQLParser::AsteriskContext>(0);
}

RQLParser::SelectListFullscanContext::SelectListFullscanContext(Select_listContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectListFullscanContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelectListFullscan(this);
}
void RQLParser::SelectListFullscanContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelectListFullscan(this);
}
RQLParser::Select_listContext* RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 26, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(192);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SelectListFullscanContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(183);
      asterisk();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SelectListContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(184);
      expression();
      setState(189);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == RQLParser::COMMA) {
        setState(185);
        match(RQLParser::COMMA);
        setState(186);
        expression();
        setState(191);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_idContext ------------------------------------------------------------------

RQLParser::Field_idContext::Field_idContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Field_idContext::getRuleIndex() const {
  return RQLParser::RuleField_id;
}

void RQLParser::Field_idContext::copyFrom(Field_idContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FieldIDUnderlineContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDUnderlineContext::UNDERLINE() {
  return getToken(RQLParser::UNDERLINE, 0);
}

tree::TerminalNode* RQLParser::FieldIDUnderlineContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::FieldIDUnderlineContext::FieldIDUnderlineContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDUnderlineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDUnderline(this);
}
void RQLParser::FieldIDUnderlineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDUnderline(this);
}
//----------------- FieldIDTableContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDTableContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::FieldIDTableContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::FieldIDTableContext::FieldIDTableContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDTableContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDTable(this);
}
void RQLParser::FieldIDTableContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDTable(this);
}
//----------------- FieldIDContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::FieldIDContext::FieldIDContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldID(this);
}
void RQLParser::FieldIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldID(this);
}
//----------------- FieldIDColumnNameContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDColumnNameContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

std::vector<tree::TerminalNode *> RQLParser::FieldIDColumnNameContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::FieldIDColumnNameContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

RQLParser::FieldIDColumnNameContext::FieldIDColumnNameContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDColumnNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDColumnName(this);
}
void RQLParser::FieldIDColumnNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDColumnName(this);
}
//----------------- FieldIDGeneratedContext ------------------------------------------------------------------

RQLParser::Gen_indexContext* RQLParser::FieldIDGeneratedContext::gen_index() {
  return getRuleContext<RQLParser::Gen_indexContext>(0);
}

tree::TerminalNode* RQLParser::FieldIDGeneratedContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::FieldIDGeneratedContext::FieldIDGeneratedContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDGeneratedContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDGenerated(this);
}
void RQLParser::FieldIDGeneratedContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDGenerated(this);
}
RQLParser::Field_idContext* RQLParser::field_id() {
  Field_idContext *_localctx = _tracker.createInstance<Field_idContext>(_ctx, getState());
  enterRule(_localctx, 28, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(211);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(194);
      antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(195);
      antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(196);
      match(RQLParser::T__0);
      setState(197);
      match(RQLParser::UNDERLINE);
      setState(198);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDColumnNameContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(199);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(200);
      match(RQLParser::DOT);
      setState(201);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(202);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(203);
      match(RQLParser::T__0);
      setState(204);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = match(RQLParser::DECIMAL);
      setState(205);
      match(RQLParser::T__1);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDGeneratedContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(206);
      antlrcpp::downCast<FieldIDGeneratedContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(207);
      match(RQLParser::T__0);
      setState(208);
      gen_index(0);
      setState(209);
      match(RQLParser::T__1);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unary_op_expressionContext ------------------------------------------------------------------

RQLParser::Unary_op_expressionContext::Unary_op_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Unary_op_expressionContext::BIT_NOT() {
  return getToken(RQLParser::BIT_NOT, 0);
}

RQLParser::ExpressionContext* RQLParser::Unary_op_expressionContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}

tree::TerminalNode* RQLParser::Unary_op_expressionContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

tree::TerminalNode* RQLParser::Unary_op_expressionContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}


size_t RQLParser::Unary_op_expressionContext::getRuleIndex() const {
  return RQLParser::RuleUnary_op_expression;
}

void RQLParser::Unary_op_expressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnary_op_expression(this);
}

void RQLParser::Unary_op_expressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnary_op_expression(this);
}

RQLParser::Unary_op_expressionContext* RQLParser::unary_op_expression() {
  Unary_op_expressionContext *_localctx = _tracker.createInstance<Unary_op_expressionContext>(_ctx, getState());
  enterRule(_localctx, 30, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(217);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(213);
        match(RQLParser::BIT_NOT);
        setState(214);
        expression();
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(215);
        antlrcpp::downCast<Unary_op_expressionContext *>(_localctx)->op = _input->LT(1);
        _la = _input->LA(1);
        if (!(_la == RQLParser::PLUS

        || _la == RQLParser::MINUS)) {
          antlrcpp::downCast<Unary_op_expressionContext *>(_localctx)->op = _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(216);
        expression();
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

//----------------- AsteriskContext ------------------------------------------------------------------

RQLParser::AsteriskContext::AsteriskContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::AsteriskContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

tree::TerminalNode* RQLParser::AsteriskContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::AsteriskContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}


size_t RQLParser::AsteriskContext::getRuleIndex() const {
  return RQLParser::RuleAsterisk;
}

void RQLParser::AsteriskContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAsterisk(this);
}

void RQLParser::AsteriskContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAsterisk(this);
}

RQLParser::AsteriskContext* RQLParser::asterisk() {
  AsteriskContext *_localctx = _tracker.createInstance<AsteriskContext>(_ctx, getState());
  enterRule(_localctx, 32, RQLParser::RuleAsterisk);
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
    setState(221);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(219);
      match(RQLParser::ID);
      setState(220);
      match(RQLParser::DOT);
    }
    setState(223);
    match(RQLParser::STAR);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

RQLParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Expression_factorContext* RQLParser::ExpressionContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}


size_t RQLParser::ExpressionContext::getRuleIndex() const {
  return RQLParser::RuleExpression;
}

void RQLParser::ExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpression(this);
}

void RQLParser::ExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpression(this);
}

RQLParser::ExpressionContext* RQLParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 34, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(225);
    expression_factor(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LogicContext ------------------------------------------------------------------

RQLParser::LogicContext::LogicContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::LogicContext::getRuleIndex() const {
  return RQLParser::RuleLogic;
}

void RQLParser::LogicContext::copyFrom(LogicContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- LogicExpressionContext ------------------------------------------------------------------

RQLParser::Expression_logicContext* RQLParser::LogicExpressionContext::expression_logic() {
  return getRuleContext<RQLParser::Expression_logicContext>(0);
}

RQLParser::LogicExpressionContext::LogicExpressionContext(LogicContext *ctx) { copyFrom(ctx); }

void RQLParser::LogicExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLogicExpression(this);
}
void RQLParser::LogicExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLogicExpression(this);
}
RQLParser::LogicContext* RQLParser::logic() {
  LogicContext *_localctx = _tracker.createInstance<LogicContext>(_ctx, getState());
  enterRule(_localctx, 36, RQLParser::RuleLogic);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::LogicExpressionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(227);
    expression_logic(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Expression_logicContext ------------------------------------------------------------------

RQLParser::Expression_logicContext::Expression_logicContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Expression_logicContext::getRuleIndex() const {
  return RQLParser::RuleExpression_logic;
}

void RQLParser::Expression_logicContext::copyFrom(Expression_logicContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpOrContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_logicContext *> RQLParser::ExpOrContext::expression_logic() {
  return getRuleContexts<RQLParser::Expression_logicContext>();
}

RQLParser::Expression_logicContext* RQLParser::ExpOrContext::expression_logic(size_t i) {
  return getRuleContext<RQLParser::Expression_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpOrContext::OR_C() {
  return getToken(RQLParser::OR_C, 0);
}

RQLParser::ExpOrContext::ExpOrContext(Expression_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpOrContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpOr(this);
}
void RQLParser::ExpOrContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpOr(this);
}
//----------------- ExpAndContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_logicContext *> RQLParser::ExpAndContext::expression_logic() {
  return getRuleContexts<RQLParser::Expression_logicContext>();
}

RQLParser::Expression_logicContext* RQLParser::ExpAndContext::expression_logic(size_t i) {
  return getRuleContext<RQLParser::Expression_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpAndContext::AND_C() {
  return getToken(RQLParser::AND_C, 0);
}

RQLParser::ExpAndContext::ExpAndContext(Expression_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpAndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpAnd(this);
}
void RQLParser::ExpAndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpAnd(this);
}
//----------------- ExpTermLogicContext ------------------------------------------------------------------

RQLParser::Term_logicContext* RQLParser::ExpTermLogicContext::term_logic() {
  return getRuleContext<RQLParser::Term_logicContext>(0);
}

RQLParser::ExpTermLogicContext::ExpTermLogicContext(Expression_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpTermLogicContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpTermLogic(this);
}
void RQLParser::ExpTermLogicContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpTermLogic(this);
}

RQLParser::Expression_logicContext* RQLParser::expression_logic() {
   return expression_logic(0);
}

RQLParser::Expression_logicContext* RQLParser::expression_logic(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Expression_logicContext *_localctx = _tracker.createInstance<Expression_logicContext>(_ctx, parentState);
  RQLParser::Expression_logicContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 38;
  enterRecursionRule(_localctx, 38, RQLParser::RuleExpression_logic, precedence);

    

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
    _localctx = _tracker.createInstance<ExpTermLogicContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(230);
    term_logic(0);
    _ctx->stop = _input->LT(-1);
    setState(240);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(238);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpAndContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(232);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(233);
          match(RQLParser::AND_C);
          setState(234);
          expression_logic(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpOrContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(235);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(236);
          match(RQLParser::OR_C);
          setState(237);
          expression_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(242);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- Term_logicContext ------------------------------------------------------------------

RQLParser::Term_logicContext::Term_logicContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Term_logicContext::getRuleIndex() const {
  return RQLParser::RuleTerm_logic;
}

void RQLParser::Term_logicContext::copyFrom(Term_logicContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpLsContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpLsContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpLsContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpLsContext::IS_LS() {
  return getToken(RQLParser::IS_LS, 0);
}

RQLParser::ExpLsContext::ExpLsContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpLsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpLs(this);
}
void RQLParser::ExpLsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpLs(this);
}
//----------------- ExpLeContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpLeContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpLeContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpLeContext::IS_LE() {
  return getToken(RQLParser::IS_LE, 0);
}

RQLParser::ExpLeContext::ExpLeContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpLeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpLe(this);
}
void RQLParser::ExpLeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpLe(this);
}
//----------------- ExpNqContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpNqContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpNqContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpNqContext::IS_NQ() {
  return getToken(RQLParser::IS_NQ, 0);
}

RQLParser::ExpNqContext::ExpNqContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpNqContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpNq(this);
}
void RQLParser::ExpNqContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpNq(this);
}
//----------------- ExpGrContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpGrContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpGrContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpGrContext::IS_GR() {
  return getToken(RQLParser::IS_GR, 0);
}

RQLParser::ExpGrContext::ExpGrContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpGrContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpGr(this);
}
void RQLParser::ExpGrContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpGr(this);
}
//----------------- ExpFactorContext ------------------------------------------------------------------

RQLParser::Expression_factorContext* RQLParser::ExpFactorContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

RQLParser::ExpFactorContext::ExpFactorContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpFactor(this);
}
void RQLParser::ExpFactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpFactor(this);
}
//----------------- ExpEqContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpEqContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpEqContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpEqContext::IS_EQ() {
  return getToken(RQLParser::IS_EQ, 0);
}

RQLParser::ExpEqContext::ExpEqContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpEqContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpEq(this);
}
void RQLParser::ExpEqContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpEq(this);
}
//----------------- ExpGeContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpGeContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpGeContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpGeContext::IS_GE() {
  return getToken(RQLParser::IS_GE, 0);
}

RQLParser::ExpGeContext::ExpGeContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpGeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpGe(this);
}
void RQLParser::ExpGeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpGe(this);
}

RQLParser::Term_logicContext* RQLParser::term_logic() {
   return term_logic(0);
}

RQLParser::Term_logicContext* RQLParser::term_logic(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Term_logicContext *_localctx = _tracker.createInstance<Term_logicContext>(_ctx, parentState);
  RQLParser::Term_logicContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 40;
  enterRecursionRule(_localctx, 40, RQLParser::RuleTerm_logic, precedence);

    

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
    _localctx = _tracker.createInstance<ExpFactorContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(244);
    expression_factor(0);
    _ctx->stop = _input->LT(-1);
    setState(266);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(264);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 26, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpEqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(246);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(247);
          match(RQLParser::IS_EQ);
          setState(248);
          term_logic(8);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpNqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(249);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(250);
          match(RQLParser::IS_NQ);
          setState(251);
          term_logic(7);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpGrContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(252);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(253);
          match(RQLParser::IS_GR);
          setState(254);
          term_logic(6);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ExpLsContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(255);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(256);
          match(RQLParser::IS_LS);
          setState(257);
          term_logic(5);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<ExpGeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(258);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(259);
          match(RQLParser::IS_GE);
          setState(260);
          term_logic(4);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<ExpLeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(261);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(262);
          match(RQLParser::IS_LE);
          setState(263);
          term_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(268);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- Expression_factorContext ------------------------------------------------------------------

RQLParser::Expression_factorContext::Expression_factorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Expression_factorContext::getRuleIndex() const {
  return RQLParser::RuleExpression_factor;
}

void RQLParser::Expression_factorContext::copyFrom(Expression_factorContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpPlusContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_factorContext *> RQLParser::ExpPlusContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext* RQLParser::ExpPlusContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

tree::TerminalNode* RQLParser::ExpPlusContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

RQLParser::ExpPlusContext::ExpPlusContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpPlusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpPlus(this);
}
void RQLParser::ExpPlusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpPlus(this);
}
//----------------- ExpMinusContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_factorContext *> RQLParser::ExpMinusContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext* RQLParser::ExpMinusContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

tree::TerminalNode* RQLParser::ExpMinusContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpMinusContext::ExpMinusContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpMinusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpMinus(this);
}
void RQLParser::ExpMinusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpMinus(this);
}
//----------------- ExpTermContext ------------------------------------------------------------------

RQLParser::TermContext* RQLParser::ExpTermContext::term() {
  return getRuleContext<RQLParser::TermContext>(0);
}

RQLParser::ExpTermContext::ExpTermContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpTermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpTerm(this);
}
void RQLParser::ExpTermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpTerm(this);
}

RQLParser::Expression_factorContext* RQLParser::expression_factor() {
   return expression_factor(0);
}

RQLParser::Expression_factorContext* RQLParser::expression_factor(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Expression_factorContext *_localctx = _tracker.createInstance<Expression_factorContext>(_ctx, parentState);
  RQLParser::Expression_factorContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 42;
  enterRecursionRule(_localctx, 42, RQLParser::RuleExpression_factor, precedence);

    

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
    _localctx = _tracker.createInstance<ExpTermContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(270);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(280);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(278);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPlusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(272);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(273);
          match(RQLParser::PLUS);
          setState(274);
          expression_factor(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMinusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(275);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(276);
          match(RQLParser::MINUS);
          setState(277);
          expression_factor(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(282);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- TermContext ------------------------------------------------------------------

RQLParser::TermContext::TermContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::TermContext::getRuleIndex() const {
  return RQLParser::RuleTerm;
}

void RQLParser::TermContext::copyFrom(TermContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpInContext ------------------------------------------------------------------

RQLParser::Expression_factorContext* RQLParser::ExpInContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

RQLParser::ExpInContext::ExpInContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpInContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpIn(this);
}
void RQLParser::ExpInContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpIn(this);
}
//----------------- ExpDecContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpDecContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::ExpDecContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpDecContext::ExpDecContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpDecContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpDec(this);
}
void RQLParser::ExpDecContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpDec(this);
}
//----------------- ExpGenIndexContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpGenIndexContext::DOLLAR() {
  return getToken(RQLParser::DOLLAR, 0);
}

RQLParser::ExpGenIndexContext::ExpGenIndexContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpGenIndexContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpGenIndex(this);
}
void RQLParser::ExpGenIndexContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpGenIndex(this);
}
//----------------- ExpAggContext ------------------------------------------------------------------

RQLParser::AgregatorContext* RQLParser::ExpAggContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

RQLParser::ExpAggContext::ExpAggContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpAggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpAgg(this);
}
void RQLParser::ExpAggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpAgg(this);
}
//----------------- ExpWindowAggContext ------------------------------------------------------------------

RQLParser::Window_aggContext* RQLParser::ExpWindowAggContext::window_agg() {
  return getRuleContext<RQLParser::Window_aggContext>(0);
}

RQLParser::ExpWindowAggContext::ExpWindowAggContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpWindowAggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpWindowAgg(this);
}
void RQLParser::ExpWindowAggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpWindowAgg(this);
}
//----------------- ExpUnaryContext ------------------------------------------------------------------

RQLParser::Unary_op_expressionContext* RQLParser::ExpUnaryContext::unary_op_expression() {
  return getRuleContext<RQLParser::Unary_op_expressionContext>(0);
}

RQLParser::ExpUnaryContext::ExpUnaryContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpUnaryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpUnary(this);
}
void RQLParser::ExpUnaryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpUnary(this);
}
//----------------- ExpMultContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpMultContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpMultContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::ExpMultContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

RQLParser::ExpMultContext::ExpMultContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpMultContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpMult(this);
}
void RQLParser::ExpMultContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpMult(this);
}
//----------------- ExpPowContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpPowContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpPowContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::ExpPowContext::BIT_XOR() {
  return getToken(RQLParser::BIT_XOR, 0);
}

RQLParser::ExpPowContext::ExpPowContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpPowContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpPow(this);
}
void RQLParser::ExpPowContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpPow(this);
}
//----------------- ExpFloatContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpFloatContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

tree::TerminalNode* RQLParser::ExpFloatContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpFloatContext::ExpFloatContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpFloat(this);
}
void RQLParser::ExpFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpFloat(this);
}
//----------------- ExpFnCallContext ------------------------------------------------------------------

RQLParser::Function_callContext* RQLParser::ExpFnCallContext::function_call() {
  return getRuleContext<RQLParser::Function_callContext>(0);
}

RQLParser::ExpFnCallContext::ExpFnCallContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFnCallContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpFnCall(this);
}
void RQLParser::ExpFnCallContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpFnCall(this);
}
//----------------- ExpDivContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpDivContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpDivContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::ExpDivContext::DIVIDE() {
  return getToken(RQLParser::DIVIDE, 0);
}

RQLParser::ExpDivContext::ExpDivContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpDivContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpDiv(this);
}
void RQLParser::ExpDivContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpDiv(this);
}
//----------------- ExpFieldContext ------------------------------------------------------------------

RQLParser::Field_idContext* RQLParser::ExpFieldContext::field_id() {
  return getRuleContext<RQLParser::Field_idContext>(0);
}

RQLParser::ExpFieldContext::ExpFieldContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFieldContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpField(this);
}
void RQLParser::ExpFieldContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpField(this);
}
//----------------- ExpNotContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpNotContext::NOT_C() {
  return getToken(RQLParser::NOT_C, 0);
}

RQLParser::TermContext* RQLParser::ExpNotContext::term() {
  return getRuleContext<RQLParser::TermContext>(0);
}

RQLParser::ExpNotContext::ExpNotContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpNotContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpNot(this);
}
void RQLParser::ExpNotContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpNot(this);
}
//----------------- ExpStringContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpStringContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

RQLParser::ExpStringContext::ExpStringContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpString(this);
}
void RQLParser::ExpStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpString(this);
}

RQLParser::TermContext* RQLParser::term() {
   return term(0);
}

RQLParser::TermContext* RQLParser::term(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::TermContext *_localctx = _tracker.createInstance<TermContext>(_ctx, parentState);
  RQLParser::TermContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 44;
  enterRecursionRule(_localctx, 44, RQLParser::RuleTerm, precedence);

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
    setState(305);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ExpInContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(284);
      match(RQLParser::T__2);
      setState(285);
      expression_factor(0);
      setState(286);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ExpFloatContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(289);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(288);
        match(RQLParser::MINUS);
      }
      setState(291);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<ExpDecContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(293);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(292);
        match(RQLParser::MINUS);
      }
      setState(295);
      match(RQLParser::DECIMAL);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExpStringContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(296);
      match(RQLParser::STRING);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<ExpUnaryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(297);
      unary_op_expression();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<ExpFieldContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(298);
      field_id();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<ExpWindowAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(299);
      window_agg();
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<ExpAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(300);
      agregator();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<ExpGenIndexContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(301);
      match(RQLParser::DOLLAR);
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<ExpFnCallContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(302);
      function_call();
      break;
    }

    case 11: {
      _localctx = _tracker.createInstance<ExpNotContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(303);
      match(RQLParser::NOT_C);
      setState(304);
      term(1);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(318);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(316);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPowContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(307);

          if (!(precpred(_ctx, 14))) throw FailedPredicateException(this, "precpred(_ctx, 14)");
          setState(308);
          match(RQLParser::BIT_XOR);
          setState(309);
          term(14);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMultContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(310);

          if (!(precpred(_ctx, 13))) throw FailedPredicateException(this, "precpred(_ctx, 13)");
          setState(311);
          match(RQLParser::STAR);
          setState(312);
          term(14);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpDivContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(313);

          if (!(precpred(_ctx, 12))) throw FailedPredicateException(this, "precpred(_ctx, 12)");
          setState(314);
          match(RQLParser::DIVIDE);
          setState(315);
          term(13);
          break;
        }

        default:
          break;
        } 
      }
      setState(320);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
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


size_t RQLParser::Stream_expressionContext::getRuleIndex() const {
  return RQLParser::RuleStream_expression;
}

void RQLParser::Stream_expressionContext::copyFrom(Stream_expressionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SExpPlusContext ------------------------------------------------------------------

std::vector<RQLParser::Stream_expressionContext *> RQLParser::SExpPlusContext::stream_expression() {
  return getRuleContexts<RQLParser::Stream_expressionContext>();
}

RQLParser::Stream_expressionContext* RQLParser::SExpPlusContext::stream_expression(size_t i) {
  return getRuleContext<RQLParser::Stream_expressionContext>(i);
}

tree::TerminalNode* RQLParser::SExpPlusContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

RQLParser::SExpPlusContext::SExpPlusContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpPlusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpPlus(this);
}
void RQLParser::SExpPlusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpPlus(this);
}
//----------------- SExpTimeMoveContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpTimeMoveContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpTimeMoveContext::IS_GR() {
  return getToken(RQLParser::IS_GR, 0);
}

tree::TerminalNode* RQLParser::SExpTimeMoveContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::SExpTimeMoveContext::SExpTimeMoveContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpTimeMoveContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpTimeMove(this);
}
void RQLParser::SExpTimeMoveContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpTimeMove(this);
}
//----------------- SExpFactorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpFactorContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

RQLParser::SExpFactorContext::SExpFactorContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpFactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpFactor(this);
}
void RQLParser::SExpFactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpFactor(this);
}
//----------------- SExpHashContext ------------------------------------------------------------------

std::vector<RQLParser::Stream_expressionContext *> RQLParser::SExpHashContext::stream_expression() {
  return getRuleContexts<RQLParser::Stream_expressionContext>();
}

RQLParser::Stream_expressionContext* RQLParser::SExpHashContext::stream_expression(size_t i) {
  return getRuleContext<RQLParser::Stream_expressionContext>(i);
}

tree::TerminalNode* RQLParser::SExpHashContext::SHARP() {
  return getToken(RQLParser::SHARP, 0);
}

RQLParser::SExpHashContext::SExpHashContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpHashContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpHash(this);
}
void RQLParser::SExpHashContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpHash(this);
}
//----------------- SExpModContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpModContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpModContext::MOD() {
  return getToken(RQLParser::MOD, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpModContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpModContext::SExpModContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpModContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpMod(this);
}
void RQLParser::SExpModContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpMod(this);
}
//----------------- SExpAgregate_proformaContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpAgregate_proformaContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpAgregate_proformaContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

RQLParser::AgregatorContext* RQLParser::SExpAgregate_proformaContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

RQLParser::SExpAgregate_proformaContext::SExpAgregate_proformaContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAgregate_proformaContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpAgregate_proforma(this);
}
void RQLParser::SExpAgregate_proformaContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpAgregate_proforma(this);
}
//----------------- SExpAgseContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpAgseContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpAgseContext::AT() {
  return getToken(RQLParser::AT, 0);
}

tree::TerminalNode* RQLParser::SExpAgseContext::COMMA() {
  return getToken(RQLParser::COMMA, 0);
}

std::vector<tree::TerminalNode *> RQLParser::SExpAgseContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::SExpAgseContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

tree::TerminalNode* RQLParser::SExpAgseContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::SExpAgseContext::SExpAgseContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAgseContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpAgse(this);
}
void RQLParser::SExpAgseContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpAgse(this);
}
//----------------- SExpMinusContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpMinusContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpMinusContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpMinusContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpMinusContext::SExpMinusContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpMinusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpMinus(this);
}
void RQLParser::SExpMinusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpMinus(this);
}
//----------------- SExpAndContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpAndContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpAndContext::AND() {
  return getToken(RQLParser::AND, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpAndContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpAndContext::SExpAndContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpAnd(this);
}
void RQLParser::SExpAndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpAnd(this);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
   return stream_expression(0);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, parentState);
  RQLParser::Stream_expressionContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 46;
  enterRecursionRule(_localctx, 46, RQLParser::RuleStream_expression, precedence);

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
    _localctx = _tracker.createInstance<SExpFactorContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(322);
    stream_factor();
    _ctx->stop = _input->LT(-1);
    setState(357);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 37, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(355);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 36, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<SExpHashContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(324);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(325);
          match(RQLParser::SHARP);
          setState(326);
          stream_expression(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<SExpPlusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(327);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(328);
          match(RQLParser::PLUS);
          setState(329);
          stream_expression(3);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<SExpAgseContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(330);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(331);
          match(RQLParser::AT);
          setState(332);
          match(RQLParser::T__2);
          setState(333);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->step = match(RQLParser::DECIMAL);
          setState(334);
          match(RQLParser::COMMA);
          setState(336);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == RQLParser::MINUS) {
            setState(335);
            match(RQLParser::MINUS);
          }
          setState(338);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->window = match(RQLParser::DECIMAL);
          setState(339);
          match(RQLParser::T__3);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<SExpAndContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(340);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(341);
          match(RQLParser::AND);
          setState(342);
          rational_se();
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<SExpModContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(343);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(344);
          match(RQLParser::MOD);
          setState(345);
          rational_se();
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<SExpTimeMoveContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(346);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(347);
          match(RQLParser::IS_GR);
          setState(348);
          match(RQLParser::DECIMAL);
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<SExpMinusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(349);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(350);
          match(RQLParser::MINUS);
          setState(351);
          rational_se();
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<SExpAgregate_proformaContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(352);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(353);
          match(RQLParser::DOT);
          setState(354);
          agregator();
          break;
        }

        default:
          break;
        } 
      }
      setState(359);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 37, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- Stream_factorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext::Stream_factorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Stream_factorContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::Gen_indexContext* RQLParser::Stream_factorContext::gen_index() {
  return getRuleContext<RQLParser::Gen_indexContext>(0);
}

RQLParser::Stream_expressionContext* RQLParser::Stream_factorContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

RQLParser::Stream_fn_callContext* RQLParser::Stream_factorContext::stream_fn_call() {
  return getRuleContext<RQLParser::Stream_fn_callContext>(0);
}


size_t RQLParser::Stream_factorContext::getRuleIndex() const {
  return RQLParser::RuleStream_factor;
}

void RQLParser::Stream_factorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStream_factor(this);
}

void RQLParser::Stream_factorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStream_factor(this);
}

RQLParser::Stream_factorContext* RQLParser::stream_factor() {
  Stream_factorContext *_localctx = _tracker.createInstance<Stream_factorContext>(_ctx, getState());
  enterRule(_localctx, 48, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(371);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 38, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(360);
      match(RQLParser::ID);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(361);
      match(RQLParser::ID);
      setState(362);
      match(RQLParser::T__0);
      setState(363);
      gen_index(0);
      setState(364);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(366);
      match(RQLParser::T__2);
      setState(367);
      stream_expression(0);
      setState(368);
      match(RQLParser::T__3);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(370);
      stream_fn_call();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Gen_indexContext ------------------------------------------------------------------

RQLParser::Gen_indexContext::Gen_indexContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Gen_indexContext *> RQLParser::Gen_indexContext::gen_index() {
  return getRuleContexts<RQLParser::Gen_indexContext>();
}

RQLParser::Gen_indexContext* RQLParser::Gen_indexContext::gen_index(size_t i) {
  return getRuleContext<RQLParser::Gen_indexContext>(i);
}

tree::TerminalNode* RQLParser::Gen_indexContext::DOLLAR() {
  return getToken(RQLParser::DOLLAR, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}


size_t RQLParser::Gen_indexContext::getRuleIndex() const {
  return RQLParser::RuleGen_index;
}

void RQLParser::Gen_indexContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterGen_index(this);
}

void RQLParser::Gen_indexContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitGen_index(this);
}


RQLParser::Gen_indexContext* RQLParser::gen_index() {
   return gen_index(0);
}

RQLParser::Gen_indexContext* RQLParser::gen_index(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Gen_indexContext *_localctx = _tracker.createInstance<Gen_indexContext>(_ctx, parentState);
  RQLParser::Gen_indexContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 50;
  enterRecursionRule(_localctx, 50, RQLParser::RuleGen_index, precedence);

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
    setState(380);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::T__2: {
        setState(374);
        match(RQLParser::T__2);
        setState(375);
        gen_index(0);
        setState(376);
        match(RQLParser::T__3);
        break;
      }

      case RQLParser::DOLLAR: {
        setState(378);
        match(RQLParser::DOLLAR);
        break;
      }

      case RQLParser::DECIMAL: {
        setState(379);
        match(RQLParser::DECIMAL);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(390);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(388);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 40, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(382);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(383);
          match(RQLParser::STAR);
          setState(384);
          gen_index(6);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(385);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(386);
          _la = _input->LA(1);
          if (!(_la == RQLParser::PLUS

          || _la == RQLParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(387);
          gen_index(5);
          break;
        }

        default:
          break;
        } 
      }
      setState(392);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx);
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


size_t RQLParser::AgregatorContext::getRuleIndex() const {
  return RQLParser::RuleAgregator;
}

void RQLParser::AgregatorContext::copyFrom(AgregatorContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- StreamMinContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamMinContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

RQLParser::StreamMinContext::StreamMinContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamMinContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamMin(this);
}
void RQLParser::StreamMinContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamMin(this);
}
//----------------- StreamAvgContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamAvgContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

RQLParser::StreamAvgContext::StreamAvgContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamAvgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamAvg(this);
}
void RQLParser::StreamAvgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamAvg(this);
}
//----------------- StreamMaxContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamMaxContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

RQLParser::StreamMaxContext::StreamMaxContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamMaxContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamMax(this);
}
void RQLParser::StreamMaxContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamMax(this);
}
//----------------- StreamSumContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamSumContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}

RQLParser::StreamSumContext::StreamSumContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamSumContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamSum(this);
}
void RQLParser::StreamSumContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamSum(this);
}
RQLParser::AgregatorContext* RQLParser::agregator() {
  AgregatorContext *_localctx = _tracker.createInstance<AgregatorContext>(_ctx, getState());
  enterRule(_localctx, 52, RQLParser::RuleAgregator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(397);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::MIN: {
        _localctx = _tracker.createInstance<RQLParser::StreamMinContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(393);
        match(RQLParser::MIN);
        break;
      }

      case RQLParser::MAX: {
        _localctx = _tracker.createInstance<RQLParser::StreamMaxContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(394);
        match(RQLParser::MAX);
        break;
      }

      case RQLParser::AVG: {
        _localctx = _tracker.createInstance<RQLParser::StreamAvgContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(395);
        match(RQLParser::AVG);
        break;
      }

      case RQLParser::SUMC: {
        _localctx = _tracker.createInstance<RQLParser::StreamSumContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(396);
        match(RQLParser::SUMC);
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

//----------------- Stream_fn_callContext ------------------------------------------------------------------

RQLParser::Stream_fn_callContext::Stream_fn_callContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Stream_expressionContext* RQLParser::Stream_fn_callContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}


size_t RQLParser::Stream_fn_callContext::getRuleIndex() const {
  return RQLParser::RuleStream_fn_call;
}

void RQLParser::Stream_fn_callContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStream_fn_call(this);
}

void RQLParser::Stream_fn_callContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStream_fn_call(this);
}

RQLParser::Stream_fn_callContext* RQLParser::stream_fn_call() {
  Stream_fn_callContext *_localctx = _tracker.createInstance<Stream_fn_callContext>(_ctx, getState());
  enterRule(_localctx, 54, RQLParser::RuleStream_fn_call);
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
    setState(399);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1030792151040) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(400);
    match(RQLParser::T__2);
    setState(401);
    stream_expression(0);
    setState(402);
    match(RQLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Window_aggContext ------------------------------------------------------------------

RQLParser::Window_aggContext::Window_aggContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Expression_factorContext* RQLParser::Window_aggContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

tree::TerminalNode* RQLParser::Window_aggContext::COLON() {
  return getToken(RQLParser::COLON, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}


size_t RQLParser::Window_aggContext::getRuleIndex() const {
  return RQLParser::RuleWindow_agg;
}

void RQLParser::Window_aggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterWindow_agg(this);
}

void RQLParser::Window_aggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitWindow_agg(this);
}

RQLParser::Window_aggContext* RQLParser::window_agg() {
  Window_aggContext *_localctx = _tracker.createInstance<Window_aggContext>(_ctx, getState());
  enterRule(_localctx, 56, RQLParser::RuleWindow_agg);
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
    setState(404);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1030792151040) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(405);
    match(RQLParser::T__2);
    setState(406);
    expression_factor(0);
    setState(407);
    match(RQLParser::COLON);
    setState(408);
    antlrcpp::downCast<Window_aggContext *>(_localctx)->width = match(RQLParser::DECIMAL);
    setState(409);
    match(RQLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Function_callContext ------------------------------------------------------------------

RQLParser::Function_callContext::Function_callContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Expression_factorContext* RQLParser::Function_callContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

tree::TerminalNode* RQLParser::Function_callContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::COLON() {
  return getToken(RQLParser::COLON, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}


size_t RQLParser::Function_callContext::getRuleIndex() const {
  return RQLParser::RuleFunction_call;
}

void RQLParser::Function_callContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFunction_call(this);
}

void RQLParser::Function_callContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFunction_call(this);
}

RQLParser::Function_callContext* RQLParser::function_call() {
  Function_callContext *_localctx = _tracker.createInstance<Function_callContext>(_ctx, getState());
  enterRule(_localctx, 58, RQLParser::RuleFunction_call);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(423);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 43, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(411);
      antlrcpp::downCast<Function_callContext *>(_localctx)->fn = match(RQLParser::ID);
      setState(412);
      match(RQLParser::T__2);
      setState(413);
      expression_factor(0);
      setState(414);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(416);
      antlrcpp::downCast<Function_callContext *>(_localctx)->fn = match(RQLParser::ID);
      setState(417);
      match(RQLParser::T__2);
      setState(418);
      expression_factor(0);
      setState(419);
      match(RQLParser::COLON);
      setState(420);
      match(RQLParser::DECIMAL);
      setState(421);
      match(RQLParser::T__3);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool RQLParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 19: return expression_logicSempred(antlrcpp::downCast<Expression_logicContext *>(context), predicateIndex);
    case 20: return term_logicSempred(antlrcpp::downCast<Term_logicContext *>(context), predicateIndex);
    case 21: return expression_factorSempred(antlrcpp::downCast<Expression_factorContext *>(context), predicateIndex);
    case 22: return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);
    case 23: return stream_expressionSempred(antlrcpp::downCast<Stream_expressionContext *>(context), predicateIndex);
    case 25: return gen_indexSempred(antlrcpp::downCast<Gen_indexContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool RQLParser::expression_logicSempred(Expression_logicContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 3);
    case 1: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::term_logicSempred(Term_logicContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2: return precpred(_ctx, 7);
    case 3: return precpred(_ctx, 6);
    case 4: return precpred(_ctx, 5);
    case 5: return precpred(_ctx, 4);
    case 6: return precpred(_ctx, 3);
    case 7: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 8: return precpred(_ctx, 3);
    case 9: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::termSempred(TermContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 10: return precpred(_ctx, 14);
    case 11: return precpred(_ctx, 13);
    case 12: return precpred(_ctx, 12);

  default:
    break;
  }
  return true;
}

bool RQLParser::stream_expressionSempred(Stream_expressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 13: return precpred(_ctx, 3);
    case 14: return precpred(_ctx, 2);
    case 15: return precpred(_ctx, 9);
    case 16: return precpred(_ctx, 8);
    case 17: return precpred(_ctx, 7);
    case 18: return precpred(_ctx, 6);
    case 19: return precpred(_ctx, 5);
    case 20: return precpred(_ctx, 4);

  default:
    break;
  }
  return true;
}

bool RQLParser::gen_indexSempred(Gen_indexContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 21: return precpred(_ctx, 5);
    case 22: return precpred(_ctx, 4);

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
