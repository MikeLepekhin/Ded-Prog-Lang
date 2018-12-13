//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_PARSER_H
#define DED_PROG_LANG_PARSER_H

#include "tree.h"

class Parser {
 private:
  const static int MAX_BUF_SIZE = 4096;

  char buf_[MAX_BUF_SIZE];
  size_t buf_size_;
  char* buf_ptr_;
  StackAllocator<Node>& allocator_;

  Node* getN() {
    double int_part_value = 0.0;
    double frac_part_value = 0.0;
    char* was_ptr_ = buf_ptr_;

    while ('0' <= *buf_ptr_ && *buf_ptr_ <= '9') {
      int_part_value = int_part_value * 10 + (*buf_ptr_ - '0');
      ++buf_ptr_;
    }
    if (buf_ptr_ == was_ptr_) {
      return nullptr;
    }
    if (*buf_ptr_ != '\0') {
      if (*buf_ptr_ != '.') {
        return allocator_.init_alloc(Node(REAL_NUMBER, int_part_value));
      }

      ++buf_ptr_;
      double cur_coeff = 0.1;

      while ('0' <= *buf_ptr_ && *buf_ptr_ <= '9') {
        frac_part_value += cur_coeff * static_cast<double>(*buf_ptr_ - '0');
        cur_coeff *= 0.1;
        ++buf_ptr_;
      }
    }
    return allocator_.init_alloc(Node(REAL_NUMBER, int_part_value + frac_part_value));
  }

  Node* getE() {
    Node* cur_node = getT();
    if (cur_node == nullptr) {
      return nullptr;
    }

    while (*buf_ptr_ == '+' || *buf_ptr_ == '-') {
      char oper = *buf_ptr_;
      ++buf_ptr_;
      Node* next_T = getT();

      if (next_T == nullptr) {
        throw IncorrectParsingException("after operator + or - should be an operand",
                                        __PRETTY_FUNCTION__);
      }
      cur_node = allocator_.init_alloc(Node(getNodeTypeByOper(oper), cur_node, next_T));
    }

    return cur_node;
  }

  Node* getT() {
    Node* cur_node = getP();
    if (cur_node == nullptr) {
      return nullptr;
    }

    while (*buf_ptr_ == '*' || *buf_ptr_ == '/') {
      char oper = *buf_ptr_;
      ++buf_ptr_;
      Node* next_P = getP();

      if (next_P == nullptr) {
        throw IncorrectParsingException("after operator * or / should be an operand",
                                        __PRETTY_FUNCTION__);
      }
      cur_node = allocator_.init_alloc(Node(getNodeTypeByOper(oper), cur_node, next_P));
    }

    return cur_node;
  }

  Node* getP() {
    if (*buf_ptr_ != '(') {
      return getN();
    }
    ++buf_ptr_;

    Node* result = result = getE();
    if (result == nullptr) {
      throw IncorrectParsingException("incorrect value for P",
                                      __PRETTY_FUNCTION__);
    }
    if (*buf_ptr_ != ')') {
      throw IncorrectParsingException("after ( should be a ) character",
                                      __PRETTY_FUNCTION__);
    }
    ++buf_ptr_;

    return result;
  }

  Node* getG() {
    if (*buf_ptr_ == '\0') {
      return nullptr;
    }
    Node* result = getE();

    if (buf_ptr_ + 1 - buf_ < buf_size_) {
      std::cerr << buf_ptr_ - buf_ << '\n';
      throw IncorrectParsingException("something gone wrong");
    }

    return result;
  }

 public:

  Parser(FILE* code_file, StackAllocator<Node>& allocator):
      allocator_(allocator) {
    buf_size_ = fread((void*)buf_, sizeof(char), MAX_BUF_SIZE - 1, code_file);
    buf_ptr_ = buf_;
    std::cout << "file size: " << buf_size_ << " bytes\n";
    printf("%s\n", buf_);
  }

  Tree makeTree()  {
    return Tree(getG());
  }
};

#endif //DED_PROG_LANG_PARSER_H
