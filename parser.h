//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_PARSER_H
#define DED_PROG_LANG_PARSER_H

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "exception.h"
#include "tree.h"
#include "lex_analyzer.h"

class Parser {
 private:
  const static int MAX_BUF_SIZE = 4096;

  std::vector<Token> tokens_;
  size_t token_ptr_;
  StackAllocator<Node>& allocator_;
  std::unordered_set<std::string> reserved_words_;

  bool compareToken(const std::string& str) {
    return !done() && tokens_[token_ptr_].value == str;
  }

  Node* getN() {
    LOG("getN");
    if (done()) {
      return nullptr;
    }
    if (tokens_[token_ptr_].token_type == DOUBLE ||
          tokens_[token_ptr_].token_type == INTEGER) {
      Node* result = allocator_.init_alloc(Node(NUMBER, atof(tokens_[token_ptr_].value.c_str())));

      ++token_ptr_;
      return result;
    }
    return nullptr;
  }

  Node* getE(int func_id) {
    LOG("getE");

    try {
      Node *cur_node = getT(func_id);
      if (cur_node == nullptr) {
        return nullptr;
      }

      while (compareToken("+") || compareToken("-")) {
        char oper = tokens_[token_ptr_].value[0];
        ++token_ptr_;
        Node *next_T = getT(func_id);

        if (next_T == nullptr) {
          throw IncorrectParsingException("after operator + or - should be an operand",
                                          __PRETTY_FUNCTION__);
        }

        std::vector<Node*> child_list{cur_node, next_T};
        cur_node = allocator_.init_alloc(Node(OPERATOR, getOperTypeByOper(oper), child_list));
      }

      return cur_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getT(int func_id) {
    LOG("getT");

    try {
      Node *cur_node = getP(func_id);
      if (cur_node == nullptr) {
        return nullptr;
      }

      while (compareToken("*") || compareToken("/")) {
        char oper = tokens_[token_ptr_].value[0];
        ++token_ptr_;
        Node *next_P = getP(func_id);

        if (next_P == nullptr) {
          throw IncorrectParsingException("after operator * or / should be an operand",
                                          __PRETTY_FUNCTION__);
        }
        std::vector<Node*> child_list{cur_node, next_P};
        cur_node = allocator_.init_alloc(Node(OPERATOR, getOperTypeByOper(oper), child_list));
      }

      return cur_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getP(int func_id) {
    LOG("getP");

    try {
      Node* expr = nullptr;

      if (compareToken("(")) {
        ++token_ptr_;
        expr = getE(func_id);
        if (expr == nullptr) {
          --token_ptr_;
          return nullptr;
        }
        if (!compareToken(")")) {
          throw IncorrectParsingException(") was expected",
                                          __PRETTY_FUNCTION__);
        }
        ++token_ptr_;
        return expr;
      }

      if (expr == nullptr) {
        expr = getN();
      }
      if (expr == nullptr) {
        expr = getId(func_id);
      }
      if (expr == nullptr) {
        expr = getSin(func_id);
      }
      if (expr == nullptr) {
        expr = getCos(func_id);
      }
      if (expr == nullptr) {
        expr = getSqrt(func_id);
      }
      /*if (expr == nullptr) {
        throw IncorrectParsingException("incorrect value for P",
                                        __PRETTY_FUNCTION__);
      }*/

      return expr;
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

  Node* getId(int func_id, bool add_var = false) {
    if (done()) {
      return nullptr;
    }
    const std::string& cur_token = tokens_[token_ptr_].value;

    if (reserved_words_.find(cur_token) != reserved_words_.end()) {
      return nullptr;
    }

    LOG("getId");
    LOG(std::to_string(token_ptr_));

    if (tokens_[token_ptr_].token_type != STRING || !isVariableNameBegin(cur_token[0])) {
      return nullptr;
    }

    bool correct_name = true;
    for (char ch: cur_token) {
      correct_name &= (isVariableNameChar(ch));
    }
    if (!correct_name) {
      return nullptr;
    }

    ++token_ptr_;
    int local_address = tree_.getVariableAddress(cur_token, func_id);
    int global_address = tree_.getVariableAddress(cur_token, -1);
    NodeType node_type = VARIABLE;

    if (!add_var && global_address != -1) {
      node_type = VARIABLE;
    } else if (!add_var && local_address != -1) {
      node_type = LOCAL_VARIABLE;
    } else if (!add_var) {
      throw IncorrectArgumentException(std::string("undefined variable: ") + cur_token,
                                       __PRETTY_FUNCTION__);
    }
    if (add_var && local_address != -1) {
      throw IncorrectArgumentException(std::string("variable redefinition: ") + cur_token,
                                       __PRETTY_FUNCTION__);
    } else if (add_var) {
      if (func_id != -1) {
        node_type = LOCAL_VARIABLE;
      }
      LOG("I want to add var");
      if (add_var) {
        local_address = tree_.addVariable(cur_token, func_id, allocator_.init_alloc(Node{NUMBER, 0}));
      }
      LOG(std::string("its address is ") + std::to_string(local_address));
      LOG("getId on finish line");
    }

    if (local_address != -1) {
      return allocator_.init_alloc(Node(node_type, local_address));
    } else {
      return allocator_.init_alloc(Node(node_type, global_address));
    }
  }

  Node* getVariableTemplate(int func_id, const std::string& type) {
    try {
      if (done()) {
        return nullptr;
      }
      const std::string& cur_token = tokens_[token_ptr_].value;

      LOG("getVariableTemplate");
      LOG(std::to_string(token_ptr_));
      if (!getStr(type)) {
        return nullptr;
      }

      Node* var_node = getId(func_id, true);
      Node* value_node = allocator_.init_alloc(Node(NUMBER, 0.0));

      if (tokens_[token_ptr_].value == "=") {
        ++token_ptr_;
        value_node = getE(func_id);
        if (value_node == nullptr) {
          throw IncorrectParsingException("after = character should be an experssion",
                                          __PRETTY_FUNCTION__);
        }
      }
      tree_.initVariable(var_node->type == VARIABLE ? -1 : func_id, var_node->value, value_node);

      LOG("variable was processed");
      return var_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getInt(int func_id) {
    try {
      LOG("getInt");
      return getVariableTemplate(func_id, "int");
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getFloat(int func_id) {
    try {
      LOG("getFloat");
      return getVariableTemplate(func_id, "float");
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getVar(int func_id) {
    try {
      LOG("getVar");
      return getVariableTemplate(func_id, "var");
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getV(int func_id) {
    LOG("getV");
    try {
      Node *node = getInt(func_id);

      if (node == nullptr) {
        node = getFloat(func_id);
      }
      if (node == nullptr) {
        node = getVar(func_id);
      }

      return node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getScan(int func_id) {
    LOG("getScan");
    LOG(std::to_string(token_ptr_));
    const std::string& cur_token = tokens_[token_ptr_].value;

    try {
      if (cur_token != "scan") {
        return nullptr;
      }
      ++token_ptr_;
      if (tokens_[token_ptr_].value != "(") {
        throw IncorrectParsingException("( was expected after scan", __PRETTY_FUNCTION__);
      }
      ++token_ptr_;
      Node *var_node = getId(func_id);

      if (var_node == nullptr) {
        throw IncorrectArgumentException("scan is available only for variables",
                                         __PRETTY_FUNCTION__);
      }
      if (tokens_[token_ptr_].value != ")") {
        throw IncorrectParsingException(") was expected after scan", __PRETTY_FUNCTION__);
      }
      ++token_ptr_;
      return allocator_.init_alloc(Node(STANDART_FUNCTION, INPUT, {var_node}));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getPrint(int func_id) {
    LOG("getPrint");
    LOG(std::to_string(token_ptr_));
    const std::string& cur_token = tokens_[token_ptr_].value;

    try {
      if (cur_token != "print") {
        return nullptr;
      }
      ++token_ptr_;
      if (tokens_[token_ptr_].value != "(") {
        throw IncorrectParsingException(std::string("( was expected after print but not ") + cur_token,
                                        __PRETTY_FUNCTION__);
      }
      ++token_ptr_;

      Node *expr_node = getE(func_id);

      if (expr_node == nullptr) {
        throw IncorrectArgumentException("scan is available only for variables",
                                         __PRETTY_FUNCTION__);
      }
      if (tokens_[token_ptr_].value != ")") {
        throw IncorrectParsingException(") was expected after print", __PRETTY_FUNCTION__);
      }
      ++token_ptr_;
      return allocator_.init_alloc(Node(STANDART_FUNCTION, OUTPUT, {expr_node}));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getA(int func_id) {
    LOG("getA");
    LOG(std::to_string(token_ptr_));

    try {
      Node *var_node = getId(func_id);
      if (!var_node) {
        return nullptr;
      }
      if (tokens_[token_ptr_].token_type != ASSIGN) {
        return nullptr;
      }
      ++token_ptr_;
      Node *expr_node = getE(func_id);

      if (expr_node == nullptr) {
        throw IncorrectParsingException("an expression was expected after =", __PRETTY_FUNCTION__);
      }

      return allocator_.init_alloc(Node(OPERATOR, EQUAL, {var_node, expr_node}));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getG(int func_id) {
    try {
      LOG("getG");
      LOG(std::to_string(token_ptr_));

      Node *node = getScan(func_id);

      if (node == nullptr) {
        node = getPrint(func_id);
      }
      if (node == nullptr) {
        node = getV(func_id);
      }
      if (node == nullptr) {
        node = getA(func_id);
      }
      if (node == nullptr) {
        node = getE(func_id);
      }

      if (node == nullptr) {
        return nullptr;
      }

      if (tokens_[token_ptr_].value != ";") {
        std::cout << token_ptr_ << '\n';
        std::cout << tokens_[token_ptr_].value << '\n';
        throw IncorrectParsingException("where ; ???", __PRETTY_FUNCTION__);
      }
      ++token_ptr_;

      return node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  bool getStr(const std::string& value) {
    if (done() || tokens_[token_ptr_].value != value) {
      return false;
    }
    ++token_ptr_;
    return true;
  }

  Node* getVarInit(int func_id) {
    Node* cur_var = getV(func_id);

    while (cur_var != nullptr) {
      if (!getStr(";")) {
        throw IncorrectParsingException("where is ; ?", __PRETTY_FUNCTION__);
      }
      cur_var = getV(func_id);
    }
    return nullptr;
  }

  Node* getFuncs() {
    return nullptr;
  }

  Node* getMain() {
    LOG("getMain");

    try {
      if (tokens_[token_ptr_].value != "main") {
        LOG("no main()");
        return nullptr;
      }
      ++token_ptr_;
      if (tokens_[token_ptr_].value != "(") {
        throw IncorrectParsingException("( was expected after main",
                                        __PRETTY_FUNCTION__);
      }
      ++token_ptr_;
      if (tokens_[token_ptr_].value != ")") {
        throw IncorrectParsingException(") was expected after main(",
                                        __PRETTY_FUNCTION__);
      }
      ++token_ptr_;

      if (!getStr("lol")) {
        LOG("no lol");
        return nullptr;
      }
      Node* main_node = allocator_.init_alloc(Node(MAIN, 0.0));
      tree_.addFunction("main", main_node);
      main_node->sons.push_back(allocator_.init_alloc(Node{VAR_INIT, 0.0}));

      Node* g_node = getG(0);
      while (g_node != nullptr) {
        main_node->sons.push_back(g_node);
        g_node = getG(0);
      }

      if (!getStr("kek")) {
        throw IncorrectParsingException("where is kek????",
                                        __PRETTY_FUNCTION__);
      }

      LOG("let's make the main node");
      return main_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getRoot() {
    LOG("getRoot");
    tree_.setRoot(allocator_.init_alloc(Node{ROOT, 0.0, {nullptr, nullptr, nullptr}}));
    tree_.getRoot()->sons[0] = allocator_.init_alloc(Node{VAR_INIT, 0.0, {}});

    Node* var_init_node = getVarInit(-1);
    tree_.getRoot()->sons[1] = getFuncs();
    tree_.getRoot()->sons[2] = getMain();
    return tree_.getRoot();
  }

  Node* getBuiltinFunc(int func_id, const std::string& str, StandartFunction func_type) {
    Node* result = allocator_.init_alloc(Node(STANDART_FUNCTION, func_type, {nullptr}));


    if (!getStr(str)) {
      return nullptr;
    }
    if (!getStr("(")) {
      throw IncorrectParsingException(std::string("was expected ( after ") + str,
                                      __PRETTY_FUNCTION__);
    }
    Node* expr_node = getE(func_id);

    if (expr_node == nullptr) {
      throw IncorrectParsingException(str + " requires an argument",
                                      __PRETTY_FUNCTION__);
    }
    if (!getStr(")")) {
      throw IncorrectParsingException(std::string("was expected _ after ") + str,
                                      __PRETTY_FUNCTION__);
    }

    result->sons[0] = expr_node;
    return result;
  }

  Node* getSin(int func_id) {
    return getBuiltinFunc(func_id, "sin", SIN);
  }

  Node* getCos(int func_id) {
    return getBuiltinFunc(func_id, "cos", COS);
  }

  Node* getSqrt(int func_id) {
    return getBuiltinFunc(func_id, "sqrt", SQ_ROOT);
  }

  Tree tree_;

 public:

  bool done() const {
    return token_ptr_ == tokens_.size();
  }

  Parser(const std::vector<Token>& tokens, StackAllocator<Node>& allocator):
      allocator_(allocator) {
    tokens_ = tokens;
  }

  Tree makeTree()  {
    tree_.setRoot(getRoot());
    return tree_;
  }
};

#endif //DED_PROG_LANG_PARSER_H
