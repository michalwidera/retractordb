#include "Parser/DESCBaseListener.h"
#include "Parser/DESCLexer.h"
#include "Parser/DESCParser.h"
#include "antlr4-runtime/antlr4-runtime.h"
#include "rdb/descriptor.h"

using namespace antlrcpp;
using namespace antlr4;

std::string status = "OK";

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Descriptor Syntax error - ";
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg;
    status = "Fail";
    exit(EPERM);
    return;
  }
};

class ParserErrorListener : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Descriptor Syntax error - ";
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg;
    status = "Fail";
    exit(EPERM);
    return;
  }
};

class ParserDESCListener : public DESCBaseListener {
  rdb::Descriptor &desc;

 public:
  ParserDESCListener(rdb::Descriptor &desc) : desc(desc){};
};

std::string parserDESCString(rdb::Descriptor &desc, std::string inlet) {
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  DESCLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  LexerErrorListener lexerErrorListener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  // Create a parser which parses the token stream
  // to create a parse tree.
  DESCParser parser(&tokens);
  ParserErrorListener parserErrorListener;
  ParserDESCListener parserDescListener(desc);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserDescListener);
  tree::ParseTree *tree = parser.desc();
  return status;
}
