#include <iostream>
#include "antlr4-runtime/antlr4-runtime.h"

#include "Parser/RQLLexer.h"
#include "Parser/RQLParser.h"
#include "Parser/RQLBaseListener.h"
// #include "Parser/RQLBaseVisitor.h"


// antlr4 -o Parser -lib Parser -encoding UTF-8 -Dlanguage=Cpp -no-listener -visitor RQLParser.g4
// https://github.com/antlr/grammars-v4/tree/master/sql/tsql
// Regenerate grammare
// make grammar && make install && xstage ../src/stage/example_5.txt
//
// Generate grammar
// cd ../src/stage && sh -c ../../scripts/antlr4call.sh RQL.g4 && cd ../../build

using namespace antlrcpp;
using namespace antlr4;
using namespace std;

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener {
public:

    void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                             const std::string &msg, std::exception_ptr e) override {
        std::cerr << "?? line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg ;
    }
};

class ParserErrorListener : public BaseErrorListener {
public:

    void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                             const std::string &msg, std::exception_ptr e) override {
        std::cerr << "?? line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg ;
    }
};

class ParserListener : public RQLBaseListener {
public:
    void exitSelect(RQLParser::SelectContext * ctx) {
        std::cout << "Select: " << ctx->select_list()->getText() << " " << std::endl ;

        for ( auto i : ctx->select_list()->select_elem() )
            std::cout << "!!  >" << i->getText() << std::endl ;
    }

    void exitDeclare(RQLParser::DeclareContext * ctx) {
        std::cout << "Declare: " << ctx->declare_list()->getText() << std::endl ;

        //for ( auto i : ctx->declare_list()->children )
        //    std::cout << "** >" << i->getText() << std::endl  ;
    }

    void exitDeclaration(RQLParser::DeclarationContext * ctx) {
        for ( auto i : ctx->declare_type() ) {
            std::cout << "Declarations: " << i->getText() << std::endl;
        }
    }
};

int main(int argc, const char *args[])
{
    ifstream ins;
    // Create the input stream.
    ins.open(args[1]);
    ANTLRInputStream input(ins);
    // Create a lexer which scans the input stream
    // to create a token stream.

    RQLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    LexerErrorListener lexerErrorListener;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexerErrorListener);

    // Print the token stream.
    //cout << "Tokens:" << endl;
    //tokens.fill();
    /*
     for (Token *token : tokens.getTokens())
    {
        std::cout << token->toString() << std::endl;
    }
    */

    // Create a parser which parses the token stream
    // to create a parse tree.

    RQLParser parser(&tokens);
    ParserErrorListener parserErrorListener;
    ParserListener parserListener;
    parser.removeParseListeners();
    parser.removeErrorListeners();
    parser.addErrorListener(&parserErrorListener);
    parser.addParseListener(&parserListener);
    tree::ParseTree *tree = parser.prog();

    // Print the parse tree in Lisp format.
    cout << endl << "Parse tree (Lisp format):" << endl;
    std::cout << tree->toStringTree(&parser) << endl;
    std::cout << endl ;
    return 0;
}