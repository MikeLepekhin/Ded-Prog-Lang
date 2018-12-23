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
#include "visualizer.h"

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

void complile(int argc, char* argv[]) {
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
  tree_file.release();

  std::string binary_filename = std::string(argv[1]) + "_binary";
  myInterpreter(argv[2], binary_filename.c_str());
}

void visualize(const std::string tree_filename) {
  SmartFile parse_tree_file(tree_filename.c_str(), "r");
  Visualizer visualizer(parse_tree_file.getFile());

  visualizer.makeTree();
  visualizer.show("prog_tree.gv", "code.lolpp");
}

int main(int argc, char* argv[]) {
  std::cout << "code file: " << argv[1] << '\n';
  std::cout << "assembler file: " << argv[2] << '\n';

  try {
    complile(argc, argv);
    std::string tree_filename = std::string(argv[1]) + "_tree";
    visualize(tree_filename);
  } catch (InterpreterException& exc) {
    std::cerr << exc;
  }

  return 0;
}