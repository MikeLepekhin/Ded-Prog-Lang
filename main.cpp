#include <cstdio>
#include <iostream>
#include <string>

#include "exception.h"
#include "normalizer.h"
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
StackAllocator<VariableBlock> Node::var_allocator_ = StackAllocator<VariableBlock>();
StackAllocator<FuncBlock> Node::func_allocator_ = StackAllocator<FuncBlock>();


int main(int argc, char* argv[]) {
  std::cout << "code file: " << argv[1] << '\n';
  std::cout << "assembler file: " << argv[2] << '\n';

  try {
    SmartFile code_file(argv[1], "r");
    SmartFile asm_file(argv[2], "w");

    std::string norm_code_name = std::string(argv[1]) + "_norm.txt";
    SmartFile norm_code(norm_code_name.c_str(), "w");
    Normalizer normalizer(code_file.getFile(), norm_code.getFile());

    normalizer.normalize();
    norm_code.release();
    code_file.release();

    SmartFile parse_file(norm_code_name.c_str(), "r");

    Parser code_parser(parse_file.getFile(), Tree::allocator_);

    Tree prog_tree = code_parser.makeTree();
    LOG("tree was built");
    prog_tree.printProgram();
    prog_tree.translateToAsm(asm_file.getFile());
    asm_file.release();


    std::string binary_name = std::string(argv[1]) + "_binary";
    myInterpreter(argv[2], binary_name.c_str());

  } catch (InterpreterException& exc) {
    std::cerr << exc;
  }

  return 0;
}