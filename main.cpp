#include <cstdio>
#include <iostream>
#include <string>

#include "exception.h"
#include "lex_analyzer.h"
#include "parser.h"
#include "tree.h"
#include "common_classes.h"

#include "assembler.h"
#include "executor.h"


void printLine(const std::string& text) {
  std::cout << "# " << text << '\n';
}

void printError(const std::string& text) {
  std::cerr << "!!! " << text << '\n';
}

void myAssembler(const char* asm_filename, const char* binary_filename) {
  std::cout << "assembler was started\n";

  SmartFile asm_file(asm_filename, "r");
  SmartFile binary_file(binary_filename, "w");

  try {
    assembly(asm_file.getFile(), binary_file.getFile());
  } catch (ProcessorException& exc) {
    std::cerr << exc;
    exit(1);
  }
}

void myExecutor(const char* binary_filename) {
  SmartFile binary_file(binary_filename);

  try {
    execute(binary_file.getFile());
  } catch (ProcessorException& exc) {
    std::cerr << exc;
    exit(1);
  }
}

void myInterpreter(const char* asm_filename, const char* binary_filename) {
  try {
    myAssembler(asm_filename, binary_filename);
    myExecutor(binary_filename);
  } catch (ProcessorException& exc) {
    std::cerr << exc;
  }
}

StackAllocator<Node> Tree::allocator_ = StackAllocator<Node>();

int main(int argc, char* argv[]) {
  std::cout << "code file: " << argv[1] << '\n';
  std::cout << "assembler file: " << argv[2] << '\n';

  try {
    SmartFile code_file(argv[1], "r");
    LexAnalyzer lex_analyzer(code_file.getFile());
    std::vector<Token> tokens;

    lex_analyzer.parseTokens(tokens);

    Parser parser(tokens, Tree::allocator_);
    Tree prog_tree = parser.makeTree();

    std::string tree_filename = std::string(argv[1]) + "_tree";
    SmartFile tree_file(tree_filename.c_str(), "w");

    SmartFile asm_file(argv[2], "w");
    prog_tree.printTree(tree_file.getFile());
    prog_tree.printAssembler(asm_file.getFile());
    asm_file.release();

    std::string binary_filename = std::string(argv[1]) + "_binary";

    myInterpreter(argv[2], binary_filename.c_str());
  } catch (InterpreterException& exc) {
    std::cerr << exc;
  }

  return 0;
}