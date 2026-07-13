#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/cerrno.hpp>
#include <boost/lexical_cast.hpp>

// please note that the order of includes is important here

#include ".antlr/RQLBaseListener.h"
#include ".antlr/RQLLexer.h"
#include ".antlr/RQLParser.h"
#include "antlr4-runtime/antlr4-runtime.h"
#include "constants.hpp"
#include "fatalError.hpp"
#include "qTree.hpp"
#include "rdb/convertTypes.hpp"

using namespace antlrcpp;
using namespace antlr4;

std::string status = "OK";

namespace {
constexpr size_t kAgseWindowSignChildIndex  = 5;
constexpr int kFallbackToStringOutputLength = 32;
}  // namespace

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Syntax error @Rql" << '\n';
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << '\n';
    std::cerr << "msg:" << msg << '\n';
    status = "Fail";
    exit(EPERM);
  }
};

class ParserErrorListener : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Syntax error @Rql" << '\n';
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << '\n';
    std::cerr << "msg:" << msg << '\n';
    status = "Fail";
    exit(EPERM);
  }
};

/* Iterator - each new field gets new fieldCount number */
int fieldCount = 0;

class ParserListener : public RQLBaseListener {
  qTree &coreInstance;

  /* Helper variable required for rational numbers processing */
  boost::rational<int> rationalResult;

  /* Helper variable required to build query or declaration */
  query qry;

  /* sequence of tokens - same variable for stream and field program*/
  std::list<token> program;

  /* Type of field */
  rdb::descFld fType = rdb::BYTE;

  /* Filed type */
  int fTypeSize = 1;

  /* Type of field - eq.1-atomic, >1 - array */
  int fTypeSizeArray = 1;

  /** Rule command support */
  std::list<token> ruleCondition;
  long int dump_left;
  long int dump_right;
  size_t dump_retention;
  std::string systemCommand;
  rule::actionType actionType;

  void recpToken(command_id id) { program.emplace_back(id); };

  template <typename T>
  void recpToken(command_id id, T arg1) {
    program.push_back(token(id, arg1));
  };

 public:
  ParserListener(qTree &coreInstance) : coreInstance(coreInstance) {};

  void enterProg(RQLParser::ProgContext *ctx) override {}

  void exitFieldID(RQLParser::FieldIDContext *ctx) override { recpToken(PUSH_ID3, ctx->getText()); }
  void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) override { recpToken(PUSH_IDX, ctx->getText()); }
  void exitFieldIDColumnName(RQLParser::FieldIDColumnNameContext *ctx) override { recpToken(PUSH_ID1, ctx->getText()); }
  void exitFieldIDTable(RQLParser::FieldIDTableContext *ctx) override { recpToken(PUSH_ID2, ctx->getText()); }

  void exitExpPlus(RQLParser::ExpPlusContext *ctx) override { recpToken(ADD); }
  void exitExpMinus(RQLParser::ExpMinusContext *ctx) override { recpToken(SUBTRACT); }
  void exitExpMult(RQLParser::ExpMultContext *ctx) override { recpToken(MULTIPLY); }
  void exitExpDiv(RQLParser::ExpDivContext *ctx) override { recpToken(DIVIDE); }
  void exitExpAnd(RQLParser::ExpAndContext *ctx) override { recpToken(AND); }
  void exitExpOr(RQLParser::ExpOrContext *ctx) override { recpToken(OR); }
  void exitExpEq(RQLParser::ExpEqContext *ctx) override { recpToken(CMP_EQUAL); }
  void exitExpNq(RQLParser::ExpNqContext *ctx) override { recpToken(CMP_NOT_EQUAL); }
  void exitExpGr(RQLParser::ExpGrContext *ctx) override { recpToken(CMP_GT); }
  void exitExpLs(RQLParser::ExpLsContext *ctx) override { recpToken(CMP_LT); }
  void exitExpGe(RQLParser::ExpGeContext *ctx) override { recpToken(CMP_GE); }
  void exitExpLe(RQLParser::ExpLeContext *ctx) override { recpToken(CMP_LE); }
  void exitExpNot(RQLParser::ExpNotContext *ctx) override { recpToken(NOT); }

  void exitExpFloat(RQLParser::ExpFloatContext *ctx) override { recpToken(PUSH_VAL, std::stof(ctx->getText())); }
  void exitExpDec(RQLParser::ExpDecContext *ctx) override { recpToken(PUSH_VAL, std::stoi(ctx->getText())); }
  void exitExpString(RQLParser::ExpStringContext *ctx) override {
    auto text = ctx->getText();
    // Strip surrounding single quotes
    if (text.size() >= 2) {
      text.erase(text.size() - 1);
      text.erase(0, 1);
    }
    program.emplace_back(PUSH_VAL, rdb::descFldVT(text));
  }
  //  void exitExpRational(RQLParser::ExpRationalContext *ctx) { program.push_back(token(PUSH_VAL, rationalResult)); }

  void exitSExpHash(RQLParser::SExpHashContext *ctx) override { recpToken(STREAM_HASH); }

  void exitSExpAnd(RQLParser::SExpAndContext *ctx) override {
    recpToken(PUSH_VAL, rationalResult);
    recpToken(STREAM_DEHASH_DIV);
  }

  void exitSExpMod(RQLParser::SExpModContext *ctx) override {
    recpToken(PUSH_VAL, rationalResult);
    recpToken(STREAM_DEHASH_MOD);
  }

  void exitStreamMin(RQLParser::StreamMinContext *ctx) override { recpToken(STREAM_MIN); }
  void exitStreamMax(RQLParser::StreamMaxContext *ctx) override { recpToken(STREAM_MAX); }
  void exitStreamAvg(RQLParser::StreamAvgContext *ctx) override { recpToken(STREAM_AVG); }
  void exitStreamSum(RQLParser::StreamSumContext *ctx) override { recpToken(STREAM_SUM); }
  void exitSExpPlus(RQLParser::SExpPlusContext *ctx) override { recpToken(STREAM_ADD); }
  void exitSExpMinus(RQLParser::SExpMinusContext *ctx) override { recpToken(STREAM_SUBTRACT, rationalResult); }

  void exitSExpAgse(RQLParser::SExpAgseContext *ctx) override {
    int window{0};
    int step{0};
    if (ctx->children[kAgseWindowSignChildIndex]->getText() == "-")
      window = -std::stoi(ctx->window->getText());
    else
      window = std::stoi(ctx->window->getText());
    step = std::stoi(ctx->step->getText());

    program.emplace_back(STREAM_AGSE, std::make_pair(step, window));
  }

  void exitFunction_call(RQLParser::Function_callContext *ctx) override {
    if (ctx->TO_STRING_FN() != nullptr && ctx->DECIMAL() != nullptr)
      recpToken(CALL2, std::make_pair(std::string("to_string"), std::stoi(ctx->DECIMAL()->getText())));
    else
      recpToken(CALL, ctx->children[0]->getText());
  }

  // page 119 - The Definitive ANTL4 Reference Guide
  void exitDeclare(RQLParser::DeclareContext *ctx) override {
    qry.filename = ctx->file_name->getText();
    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);
    qry.id           = ctx->ID()->getText();
    qry.rInterval    = rationalResult;
    qry.isDisposable = (ctx->DISPOSABLE() != nullptr);
    qry.isOneShot    = (ctx->ONESHOT() != nullptr);
    qry.isHold       = (ctx->HOLD() != nullptr);
    coreInstance.push_back(qry);
    qry.reset();
    fieldCount = 0;
  }

  // https://www.programiz.com/cpp-programming/string-float-conversion
  // https://www.geeksforgeeks.org/converting-strings-numbers-cc/

  void exitRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) override {
    rationalResult = Rationalize(std::stod(ctx->FLOAT()->getText()));
  }

  void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) override {
    rationalResult = std::stoi(ctx->DECIMAL()->getText());
  }

  void exitFraction(RQLParser::FractionContext *ctx) override {
    const int nom = std::stoi(ctx->children[0]->getText());
    const int den = std::stoi(ctx->children[2]->getText());
    if (den == 0) FatalError("RQLParser::exitFraction: denominator is zero");
    rationalResult = boost::rational<int>(nom, den);
  }

  void exitSelect(RQLParser::SelectContext *ctx) override {
    // this loop creates field names in streamName + "_" + counter++
    for (auto &i : qry.lSchema) {
      if ((i.field_.rname).starts_with("_")) (i.field_.rname) = ctx->ID()->getText() + i.field_.rname;
    }

    qry.id = ctx->ID()->getText();

    if (qry.id == constants::Reserved_id_oob) {
      std::cerr << "Error: " << constants::Reserved_id_oob << " is reserved stream name." << '\n';
      SPDLOG_ERROR("{} is reserved stream name.", constants::Reserved_id_oob);
      abort();
    }

    qry.lProgram = program;
    if (ctx->VOLATILE() != nullptr) {
      qry.policy = std::make_pair("MEMORY", 1);
    }

    if (ctx->FILE() != nullptr) {
      qry.filename = ctx->file_name->getText();

      // This removes ''
      qry.filename.erase(qry.filename.size() - 1);
      qry.filename.erase(0, 1);

      if (qry.filename.empty()) FatalError("RQLParser: directive filename must not be empty");
    }

    if (ctx->STORAGE() != nullptr) {
      qry.storage_policy = ctx->type_name->getText();
      std::ranges::transform(qry.storage_policy, qry.storage_policy.begin(), ::toupper);  // to upper case
    }

    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  void exitRetention(RQLParser::RetentionContext *ctx) override {
    if (ctx->segments != nullptr) {
      // retention {capacity} !{segments}
      qry.retention = std::pair<int, int>(      //
          std::stoi(ctx->segments->getText()),  //
          std::stoi(ctx->capacity->getText()));
    } else {
      // retention {capacity} - note: segments is optional but capacity is required
      qry.policy.second = std::stoi(ctx->capacity->getText());
    }
  }

  void exitRulez(RQLParser::RulezContext *ctx) override {
    std::string stream_name(ctx->stream_name->getText());
    rule ruleConstruct(rule(ctx->name->getText(), ruleCondition));

    for (auto &i : coreInstance) {
      if (i.id == stream_name) {
        if (i.isDeclaration()) {
          std::cerr << "Error: Cannot attach rule to declaration stream: " << stream_name << " Rule: " << ctx->name->getText()
                    << '\n';
          SPDLOG_ERROR("Parser/Rule: Cannot attach rule to declaration stream: {} Rule: {}", stream_name, ctx->name->getText());
          abort();
        }
        if (actionType == rule::DUMP) {
          ruleConstruct.action    = rule::DUMP;
          ruleConstruct.dumpRange = std::make_pair(dump_left, dump_right);
          if (dump_left > dump_right) {
            std::cerr << "Error: Dump left range cannot be greater than dump right range" << '\n';
            SPDLOG_ERROR("Parser/Rule: Dump left range cannot be greater than dump right range");
            abort();
          }
          ruleConstruct.dump_retention = dump_retention;
        } else if (actionType == rule::SYSTEM) {
          ruleConstruct.action        = rule::SYSTEM;
          ruleConstruct.systemCommand = systemCommand;
        } else {
          std::cerr << "Error: Unknown action type: " << std::to_string(actionType) << " stream_name: " << stream_name
                    << " Rule: " << ctx->name->getText() << '\n';
          SPDLOG_ERROR("Parser/Rule: Unknown action type: {} stream_name: {} Rule: {}", std::to_string(actionType), stream_name,
                       ctx->name->getText());
          abort();
        }

        i.lRules.push_back(ruleConstruct);
        break;
      }
    }
    program.clear();
    dump_left      = 0;
    dump_right     = 0;
    dump_retention = 0;
    systemCommand.clear();
    ruleCondition.clear();
    actionType = rule::UNKNOWN_ACTION;
    qry.reset();
    fieldCount = 0;
  }

  void exitDumppart(RQLParser::DumppartContext *ctx) override {
    actionType = rule::DUMP;
    dump_left  = std::stoi(ctx->step_back->getText());
    if (ctx->children[1]->getText() == "-") dump_left = -dump_left;
    dump_right = std::stoi(ctx->step_forward->getText());
    if (ctx->children[4]->getText() == "-" || ctx->children[3]->getText() == "−") dump_right = -dump_right;

    if (ctx->rule_retnetion != nullptr)
      dump_retention = std::stoi(ctx->rule_retnetion->getText());
    else
      dump_retention = 0;  // Default: no retention
  }

  void exitSystempart(RQLParser::SystempartContext *ctx) override {
    actionType    = rule::SYSTEM;
    systemCommand = ctx->syscmd->getText();
    // This removes ''
    systemCommand.erase(systemCommand.size() - 1);
    systemCommand.erase(0, 1);
  }

  void exitCoption(RQLParser::CoptionContext *ctx) override {
    qry.id = ":" + ctx->directive->getText();
    std::ranges::transform(qry.id, qry.id.begin(), ::toupper);  // to upper case
    qry.filename = ctx->value->getText();

    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);

    if (qry.filename.empty()) FatalError("RQLParser: directive filename must not be empty");

    // Add / at the end of path, if not present in case of STORAGE
    if (qry.id == ":STORAGE" && qry.filename[qry.filename.size() - 1] != '/') qry.filename.push_back('/');

    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  void exitSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) override {
    recpToken(STREAM_TIMEMOVE, std::stoi(ctx->DECIMAL()->getText()));
  }

  void exitStream_factor(RQLParser::Stream_factorContext *ctx) override {
    if (ctx->children.size() == 1) program.emplace_back(PUSH_STREAM, ctx->ID()->getText());
  }

  void exitSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) override {
    recpToken(PUSH_TSCAN, ctx->getText());
    qry.lSchema.emplace_back(rdb::rField(/*Field_*/ "_" + boost::lexical_cast<std::string>(fieldCount++), 4, 1, rdb::INTEGER),
                             program);
    program.clear();
  }

  void exitLogicExpression(RQLParser::LogicExpressionContext *ctx) override {
    ruleCondition = program;
    program.clear();
  }

  void exitExpression(RQLParser::ExpressionContext *ctx) override {
    auto outType = rdb::INTEGER;
    int outLen   = 4;
    int outArr   = 1;
    for (auto &it : program) {
      if ((it.getCommandID() == CALL || it.getCommandID() == CALL2) && it.getStr_() == "to_string") {
        outType = rdb::STRING;
        outLen  = 1;
        break;
      }
      if (it.getCommandID() == PUSH_VAL && it.getVT().index() == rdb::STRING) {
        outType = rdb::STRING;
        outLen  = 1;
        break;
      }
    }
    if (outType == rdb::STRING) {
      outArr = 0;
      for (auto &tk : program) {
        if (tk.getCommandID() == CALL2 && tk.getStr_() == "to_string")
          outArr += std::get<std::pair<std::string, int>>(tk.getVT()).second;
        else if (tk.getCommandID() == CALL && tk.getStr_() == "to_string")
          outArr += kFallbackToStringOutputLength;
        else if (tk.getCommandID() == PUSH_VAL && tk.getVT().index() == rdb::STRING)
          outArr += static_cast<int>(std::get<std::string>(tk.getVT()).length());
      }
      if (outArr == 0) outArr = kFallbackToStringOutputLength;
    }
    if (outType == rdb::INTEGER && !program.empty()) {
      auto &last = program.back();
      if (last.getCommandID() == CALL) {
        auto fn = last.getStr_();
        if (fn == "to_float") {
          outType = rdb::FLOAT;
          outLen  = 4;
        } else if (fn == "to_double") {
          outType = rdb::DOUBLE;
          outLen  = static_cast<int>(sizeof(double));
        }
      }
    }
    qry.lSchema.emplace_back(
        rdb::rField(/*Field_*/ "_" + boost::lexical_cast<std::string>(fieldCount++), outLen, outArr, outType), program);
    program.clear();
  }

  void exitTypeString(RQLParser::TypeStringContext *ctx) override {
    fType     = rdb::STRING;
    fTypeSize = sizeof(uint8_t);
  }

  void exitTypeByte(RQLParser::TypeByteContext *ctx) override {
    fType     = rdb::BYTE;
    fTypeSize = sizeof(uint8_t);
  }
  void exitTypeInt(RQLParser::TypeIntContext *ctx) override {
    fType     = rdb::INTEGER;
    fTypeSize = sizeof(int);
  }
  void exitTypeUnsigned(RQLParser::TypeUnsignedContext *ctx) override {
    fType     = rdb::UINT;
    fTypeSize = sizeof(unsigned);
  }
  void exitTypeFloat(RQLParser::TypeFloatContext *ctx) override {
    fType     = rdb::FLOAT;
    fTypeSize = sizeof(float);
  }
  void exitTypeDouble(RQLParser::TypeDoubleContext *ctx) override {
    fType     = rdb::DOUBLE;
    fTypeSize = sizeof(double);
  }

  void exitSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) override {
    auto fTypeSizeArray = 1;  // Default:1
    if (ctx->type_size != nullptr) fTypeSizeArray = std::stoi(ctx->type_size->getText());
    std::list<token> emptyProgram;
    qry.lSchema.emplace_back(rdb::rField(ctx->ID()->getText(), fTypeSize, fTypeSizeArray, fType), emptyProgram);
    fType = rdb::BYTE;
  }
};

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet) {
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  RQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  LexerErrorListener lexerErrorListener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  // Create a parser which parses the token stream
  // to create a parse tree.
  RQLParser parser(&tokens);
  ParserErrorListener parserErrorListener;
  ParserListener parserListener(coreInstance);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserListener);
  tree::ParseTree *tree  = parser.prog();
  std::string firsttoken = "UNRECOGNIZED";
  if (!tree->children.empty() && !tree->children[0]->children.empty()) firsttoken = tree->children[0]->children[0]->getText();
  std::ranges::transform(firsttoken, firsttoken.begin(), ::toupper);

  std::string streamName;  // tree->children[1]->children[0]->getText();
  if (!tree->children.empty()) {
    if (auto *selectCtx = dynamic_cast<RQLParser::SelectContext *>(tree->children[0])) {
      streamName = selectCtx->ID()->getText();
    } else if (auto *declareCtx = dynamic_cast<RQLParser::DeclareContext *>(tree->children[0])) {
      streamName = declareCtx->stream_name->getText();
    } else if (auto *ruleCtx = dynamic_cast<RQLParser::RulezContext *>(tree->children[0])) {
      streamName = ruleCtx->stream_name->getText();
    }
  }
  return {status, firsttoken, streamName};
}

std::vector<std::string> readLogicalLines(std::ifstream &file) {
  std::vector<std::string> result;
  std::string line;
  std::string accumulated;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;
    if (line.back() == '\\') {
      accumulated += line.substr(0, line.size() - 1) + ' ';
      continue;
    }
    accumulated += line;
    result.push_back(std::move(accumulated));
    accumulated = {};
  }
  return result;
}

std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile) {
  std::ifstream file(sInputFile);
  if (!file.is_open()) {
    SPDLOG_ERROR("Error: Unable to open file!");
    return "Unable to open file.";
  }

  std::string status = "Empty file.";
  for (const auto &stmt : readLogicalLines(file)) {
    auto [result, first_keyword, stream_name] = parserRQLString(coreInstance, stmt);
    status                                    = result;
    if (status != "OK") {
      SPDLOG_ERROR("Error: Parsing failed on {}.\n{}", first_keyword, stmt);
      return status;
    }
  }

  return status;
}
