#include <iostream>
#include "antlr4-runtime/antlr4-runtime.h"

#include "Parser/RQLLexer.h"
#include "Parser/RQLParser.h"

// antlr4 -o Parser -lib Parser -encoding UTF-8 -Dlanguage=Cpp -no-listener -visitor RQLParser.g4
// https://github.com/antlr/grammars-v4/tree/master/sql/tsql
// make grammar && make install && xstage ../src/stage/example_1.txt

using namespace antlrcpp;
using namespace antlr4;
using namespace std;
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
    tree::ParseTree *tree = parser.prog();
    // Print the parse tree in Lisp format.
    cout << endl << "Parse tree (Lisp format):" << endl;
    std::cout << tree->toStringTree(&parser) << endl;
    std::cout << endl ;
    return 0;
}