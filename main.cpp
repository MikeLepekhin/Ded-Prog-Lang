#include <cstdio>
#include <iostream>

#include "exception.h"
#include "parser.h"
#include "tree.h"
#include "common_classes.h"

StackAllocator<Node> Tree::allocator_ = StackAllocator<Node>();

int main(int argc, char* argv[]) {
  std::cout << "code file: " << argv[1] << '\n';
  std::cout << "assembler file: " << argv[2] << '\n';

  try {
    SmartFile code(argv[1], "r");
    Parser code_parser(code.getFile(), Tree::allocator_);

    Tree prog_tree = code_parser.makeTree();

    prog_tree.printInfixNotation();
  } catch (InterpreterException& exc) {
    std::cerr << exc;
  }

  return 0;
}