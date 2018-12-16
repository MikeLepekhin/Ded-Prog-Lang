//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_PARSER_H
#define DED_PROG_LANG_PARSER_H

#include <iostream>
#include <string>
#include <unordered_set>

#include "exception.h"
#include "tree.h"

class Parser {
 private:
  const static int MAX_BUF_SIZE = 4096;

  char buf_[MAX_BUF_SIZE];
  size_t buf_size_;
  char* buf_ptr_;
  StackAllocator<Node>& allocator_;
  std::unordered_set<std::string> reserved_words_;

  Node* getN() {
    LOG("getN");
    try {
      double int_part_value = 0.0;
      double frac_part_value = 0.0;
      char *was_ptr_ = buf_ptr_;

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
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getE() {
    LOG("getE");

    try {
      Node *cur_node = getT();
      if (cur_node == nullptr) {
        return nullptr;
      }

      while (*buf_ptr_ == '+' || *buf_ptr_ == '-') {
        char oper = *buf_ptr_;
        ++buf_ptr_;
        Node *next_T = getT();

        if (next_T == nullptr) {
          throw IncorrectParsingException("after operator + or - should be an operand",
                                          __PRETTY_FUNCTION__);
        }
        cur_node = allocator_.init_alloc(Node(getNodeTypeByOper(oper), cur_node, next_T));
      }

      return cur_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getT() {
    LOG("getT");

    try {
      Node *cur_node = getP();
      if (cur_node == nullptr) {
        return nullptr;
      }

      while (*buf_ptr_ == '*' || *buf_ptr_ == '/') {
        char oper = *buf_ptr_;
        ++buf_ptr_;
        Node *next_P = getP();

        if (next_P == nullptr) {
          throw IncorrectParsingException("after operator * or / should be an operand",
                                          __PRETTY_FUNCTION__);
        }
        cur_node = allocator_.init_alloc(Node(getNodeTypeByOper(oper), cur_node, next_P));
      }

      return cur_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getP() {
    LOG("getP");

    try {
      if (*buf_ptr_ != '(') {
        Node *expr = getN();

        if (expr == nullptr) {
          return getId();
        }
        return expr;
      }
      ++buf_ptr_;

      Node *result = result = getE();
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
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  bool isVariableNameBegin(char ch) const {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
  }

  bool isVariableNameChar(char ch) const {
    return isVariableNameBegin(ch) || ch == '_' || isdigit(ch);
  }

  Node* getId() {
    LOG("getId");
    if (!isVariableNameBegin(*buf_ptr_)) {
      return nullptr;
    }

    char* was_ptr = buf_ptr_;
    std::string var_name = "";
    while (isVariableNameChar(*buf_ptr_)) {
      var_name.push_back(*buf_ptr_);
      ++buf_ptr_;
    }

    if (reserved_words_.find(var_name) != reserved_words_.end()) {
      buf_ptr_ = was_ptr;
      return nullptr;
    }
    LOG("getId on finish line");
    return allocator_.init_alloc(Node(VARIABLE, var_name));
  }

  Node* getVariableTemplate(const std::string& type) {
    try {
      LOG("getVariableTemplate");
      if (!getStr(type + " ")) {
        return nullptr;
      }
      LOG(type + " was read");
      Node *var_node = getId();
      var_node->type = V_NODE;

      if (*buf_ptr_ == '=') {
        ++buf_ptr_;
        Node *var_value = getE();
        if (var_value == nullptr) {
          throw IncorrectParsingException("after = character should be an experssion",
                                          __PRETTY_FUNCTION__);
        }
        var_node->left_son = var_value;
      }
      LOG("variable was processed");
      return var_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getInt() {
    LOG("getInt");
    return getVariableTemplate("int");
  }

  Node* getFloat() {
    LOG("getFloat");
    return getVariableTemplate("float");
  }

  Node* getVar() {
    LOG("getVar");
    return getVariableTemplate("var");
  }

  Node* getV() {
    LOG("getV");
    try {
      Node *node = getInt();

      if (node == nullptr) {
        node = getFloat();
      }
      if (node == nullptr) {
        node = getVar();
      }

      return node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getScan() {
    LOG("getScan");
    try {
      if (!getStr("scan(")) {
        return nullptr;
      }
      Node *var_node = getId();

      if (var_node == nullptr) {
        throw IncorrectArgumentException("scan is available only for variables",
                                         __PRETTY_FUNCTION__);
      }
      if (!getStr(")")) {
        throw IncorrectParsingException("say what?",
                                        __PRETTY_FUNCTION__);
      }
      return allocator_.init_alloc(Node(SCAN_NODE, var_node));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getPrint() {
    LOG("getPrint");
    try {
      if (!getStr("print(")) {
        return nullptr;
      }
      Node *expr_node = getE();

      if (expr_node == nullptr) {
        throw IncorrectArgumentException("scan is available only for variables",
                                         __PRETTY_FUNCTION__);
      }
      if (!getStr(")")) {
        throw IncorrectParsingException("say what?",
                                        __PRETTY_FUNCTION__);
      }
      return allocator_.init_alloc(Node(PRINT_NODE, expr_node));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  bool getStr(const std::string& str) {
    LOG("getStr");
    size_t char_id = 0;
    if (str == "kek\n") {
      LOG(std::string("") + *buf_ptr_);
    }

    while (*buf_ptr_ != '\0' && char_id < str.size() && *(buf_ptr_ + char_id) == str[char_id]) {
      ++char_id;
    }
    if (str == "kek\n") {
      LOG(std::to_string(char_id) + " chars was read");
    }
    if (char_id < str.size()) {
      return false;
    }
    buf_ptr_ += str.size();
    return true;
  }

  Node* getA() {
    try {
      char *was_buf = buf_ptr_;

      Node *var_node = getId();
      if (!var_node) {
        buf_ptr_ = was_buf;
        return nullptr;
      }
      if (!getStr("=")) {
        buf_ptr_ = was_buf;
        return nullptr;
      }
      Node *expr_node = getE();

      if (expr_node == nullptr) {
        buf_ptr_ = was_buf;
        return nullptr;
      }

      return allocator_.init_alloc(Node(ASSIGN_NODE, var_node->getVarName(), expr_node));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getG() {
    try {
      char* was_ptr = buf_ptr_;
      Node *node = getScan();

      if (node == nullptr) {
        node = getPrint();
      }
      if (node == nullptr) {
        node = getV();
      }
      if (node == nullptr) {
        node = getA();
      }
      if (node == nullptr) {
        node = getE();
      }

      if (!getStr(";\n")) {
        LOG("where ; ???");
        buf_ptr_ = was_ptr;
        return nullptr;
      }

      return node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getFuncBody() {
    try {
      Node *left_son = getG();

      if (left_son == nullptr) {
        return nullptr;
      }

      Node *right_son = getFuncBody();

      return allocator_.init_alloc(Node(FUNC_BODY_NODE, left_son, right_son));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getMain() {
    LOG("getMain");

    try {
      if (!getStr("main()\n")) {
        LOG("no main()");
        return nullptr;
      }
      if (!getStr("lol\n")) {
        LOG("no lol");
        return nullptr;
      }
      Node *body = getFuncBody();
      LOG("body was parsed");
      if (!getStr("kek\n")) {
        LOG("no kek");
        return nullptr;
      }

      LOG("let's make the main node");
      return allocator_.init_alloc(Node(FUNC_NODE, "main", body));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getProg() {
    LOG("getProg");

    return getMain();
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
    reserved_words_.insert("lol");
    reserved_words_.insert("kek");
    reserved_words_.insert("main");
    reserved_words_.insert("sin");
    reserved_words_.insert("cos");
    reserved_words_.insert("exp");
    reserved_words_.insert("power");
    reserved_words_.insert("func");
    reserved_words_.insert("var");
    reserved_words_.insert("int");
    reserved_words_.insert("float");
    reserved_words_.insert("sqrt");

    return Tree(getProg());
  }
};

#endif //DED_PROG_LANG_PARSER_H
