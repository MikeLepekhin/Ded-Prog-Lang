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
    if (done()) {
      return nullptr;
    }

    try {
      Node *cur_node = getT(func_id);
      if (cur_node == nullptr) {
        return nullptr;
      }

      while (compareToken("+") || compareToken("-") || compareToken("||") || compareToken("/") ||
             compareToken("==") || compareToken("!=") || compareToken("<=") || compareToken(">=") ||
             compareToken("<") || compareToken(">")) {

        std::string oper = tokens_[token_ptr_].value;
        ++token_ptr_;

        Node* next_operand = nullptr;

        if (oper == "||") {
          next_operand = getE(func_id);
        } else {
          next_operand = getT(func_id);
        }

        if (next_operand == nullptr) {
          throw IncorrectParsingException("after operator + or - should be an operand",
                                          __PRETTY_FUNCTION__);
        }

        cur_node = allocator_.init_alloc(Node(OPERATOR, getOperTypeByOper(oper), {cur_node, next_operand}));
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

      while (compareToken("*") || compareToken("/") || compareToken("&&")) {
        std::string oper = tokens_[token_ptr_].value;
        ++token_ptr_;
        Node* next_operand = nullptr;

        if (oper == "||") {
          next_operand = getE(func_id);
        } else {
          next_operand = getP(func_id);
        }

        if (next_operand == nullptr) {
          throw IncorrectParsingException("after operator * or / should be an operand",
                                          __PRETTY_FUNCTION__);
        }
        cur_node = allocator_.init_alloc(Node(OPERATOR, getOperTypeByOper(oper), {cur_node, next_operand}));
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

        LOG(token_ptr_);
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
        expr = getParam(func_id, false);
      }
      if (expr == nullptr) {
        expr = getId(func_id);
      }
      if (expr == nullptr) {
        expr = getFuncCall(func_id);
      }

      return expr;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getFuncCall(int func_id) {
    Node* func_node = nullptr;

    if (func_node == nullptr) {
      func_node = getSin(func_id);
    }
    if (func_node == nullptr) {
      func_node = getCos(func_id);
    }
    if (func_node == nullptr) {
      func_node = getSqrt(func_id);
    }
    if (func_node == nullptr) {
      func_node = getFuncHeader(false);
    }
    return func_node;
  }

  bool isVariableNameBegin(char ch) const {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
  }

  bool isVariableNameChar(char ch) const {
    return isVariableNameBegin(ch) || ch == '_' || isdigit(ch);
  }

  bool isCorrectVariable(const std::string& str) const {
    bool correct_name = isVariableNameBegin(str[0]);

    for (char ch: str) {
      correct_name &= (isVariableNameChar(ch));
    }
    return correct_name;
  }

  Node* getId(int func_id, bool add_var = false) {
    if (done() || tokens_[token_ptr_].token_type == KEYWORD) {
      return nullptr;
    }
    const std::string& cur_token = tokens_[token_ptr_].value;

    LOG("getId");
    LOG(std::to_string(token_ptr_));

    if (!isCorrectVariable(cur_token)) {
      return nullptr;
    }

    int local_address = tree_.getVariableAddress(cur_token, func_id);
    int global_address = tree_.getVariableAddress(cur_token, -1);
    NodeType node_type = VARIABLE;

    if (!add_var && global_address == -1 && local_address == -1) {
      return nullptr;
    }
    ++token_ptr_;
    if (!add_var && global_address != -1) {
      node_type = VARIABLE;
    } else if (!add_var && local_address != -1) {
      node_type = LOCAL_VARIABLE;
    } else if (add_var && local_address != -1) {
      throw IncorrectArgumentException(std::string("variable redefinition: ") + cur_token,
                                       __PRETTY_FUNCTION__);
    } else if (add_var) {
      if (func_id != -1) {
        node_type = LOCAL_VARIABLE;
      }
      LOG("I want to add var");
      local_address = tree_.addVariable(cur_token, func_id, allocator_.init_alloc(Node{NUMBER, 0}));
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

      LOG("getVariableTemplate");
      LOG(std::to_string(token_ptr_));
      if (!getStr(type)) {
        return nullptr;
      }

      Node* var_node = getId(func_id, true);
      Node* value_node = allocator_.init_alloc(Node(NUMBER, 0.0));

      if (var_node == nullptr) {
        throw IncorrectParsingException(std::string("after ") + type + " should be a variable name",
                                        __PRETTY_FUNCTION__);
      }

      if (tokens_[token_ptr_].value == "=") {
        ++token_ptr_;
        value_node = getE(func_id);
        if (value_node == nullptr) {
          throw IncorrectParsingException("after = character should be an expression",
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

    try {
      if (!getStr("scan")) {
        return nullptr;
      }
      if (!getStr("(")) {
        throw IncorrectParsingException("( was expected after scan", __PRETTY_FUNCTION__);
      }
      Node *var_node = getId(func_id);

      if (var_node == nullptr) {
        throw IncorrectArgumentException("scan is available only for variables",
                                         __PRETTY_FUNCTION__);
      }
      if (!getStr(")")) {
        throw IncorrectParsingException(") was expected after scan", __PRETTY_FUNCTION__);
      }
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
        throw IncorrectArgumentException("print requires an expression",
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
    if (done() || tokens_[token_ptr_].token_type == KEYWORD) {
      return nullptr;
    }

    LOG("getA");
    LOG(std::to_string(token_ptr_));
    LOG(std::string(tokens_[token_ptr_].value));

    try {
      Node* var_node = getId(func_id);
      if (!var_node) {
        return nullptr;
      }

      LangOperator oper_type = EQUAL;

      if (getStr("=")) {
        oper_type = EQUAL;
      } else if (getStr("+=")) {
        oper_type = PLUS_EQUAL;
      } else if (getStr("-=")) {
        oper_type = MINUS_EQUAL;
      } else if (getStr("*=")) {
        oper_type = MULTIPLY_EQUAL;
      } else if (getStr("/=")) {
        oper_type = DIVIDE_EQUAL;
      } else {
        return nullptr;
      }
      Node* expr_node = getE(func_id);

      if (expr_node == nullptr) {
        LOG(std::to_string(token_ptr_));
        LOG(std::string(tokens_[token_ptr_].value));
        throw IncorrectParsingException("an expression was expected after =", __PRETTY_FUNCTION__);
      }

      return allocator_.init_alloc(Node(OPERATOR, oper_type, {var_node, expr_node}));
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getReturn(int func_id) {
    LOG("getReturn");

    if (!getStr("return")) {
      return nullptr;
    }
    Node* result_node = getE(func_id);
    if (result_node == nullptr) {
      return allocator_.init_alloc(Node{RETURN, 0});
    } else {
      return allocator_.init_alloc(Node{RETURN, 0, {result_node}});
    }
  }

  Node* getG(int func_id) {
    try {
      LOG("getG");
      LOG(std::to_string(token_ptr_));

      if (done() || tokens_[token_ptr_].value == "lol" ||
        tokens_[token_ptr_].value == "kek") {
        return nullptr;
      }

      Node *node = getScan(func_id);

      if (node == nullptr) {
        node = getPrint(func_id);
      }
      if (node == nullptr) {
        node = getReturn(func_id);
      }
      if (node == nullptr) {
        node = getV(func_id);
      }
      if (node == nullptr) {
        node = getA(func_id);
      }
      if (node == nullptr) {
        node = getLogic(func_id, "if");
      }
      if (node == nullptr) {
        node = getLogic(func_id, "while");
      }
      if (node == nullptr) {
        node = getFuncCall(func_id);
      }
      if (node == nullptr) {
        node = getE(func_id);
      }

      if (node == nullptr) {
        return nullptr;
      }

      if (node->type != LOGIC && !getStr(";")) {
        std::cout << token_ptr_ << '\n';
        std::cout << tokens_[token_ptr_].value << '\n';
        throw IncorrectParsingException("where ; ???", __PRETTY_FUNCTION__);
      }

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
    if (func_id == -1) {
      tree_.getRoot()->sons[0] = allocator_.init_alloc(Node{VAR_INIT, 0});
    }

    Node* cur_var = getV(func_id);

    while (cur_var != nullptr) {
      if (!getStr(";")) {
        throw IncorrectParsingException("where is ; ?", __PRETTY_FUNCTION__);
      }
      cur_var = getV(func_id);
    }
    return nullptr;
  }

  Node* getElse(int func_id) {
    if (!getStr("else")) {
      return nullptr;
    }
    if (!getStr("lol")) {
      throw IncorrectParsingException("where is lol???", __PRETTY_FUNCTION__);
    }
    Node* else_node = allocator_.init_alloc(Node{LOGIC, ELSE});
    Node* g_node = getG(func_id);

    while (g_node != nullptr) {
      LOG("getG from else");
      else_node->sons.push_back(g_node);
      g_node = getG(func_id);
    }
    if (!getStr("kek")) {
      throw IncorrectParsingException("where is kek???", __PRETTY_FUNCTION__);
    }
    return else_node;
  }

  Node* getLogic(int func_id, const std::string& oper) {
    try {
      if (!getStr(oper)) {
        return nullptr;
      }

      if (!getStr("(")) {
        throw IncorrectParsingException(std::string("( was expected after ") + oper,
                                        __PRETTY_FUNCTION__);
      }

      Node *expr_node = getE(func_id);

      if (expr_node == nullptr) {
        throw IncorrectParsingException(std::string("logic expression was expected in") + oper,
                                        __PRETTY_FUNCTION__);
      }

      Node* condition_node = allocator_.init_alloc(Node{LOGIC, CONDITION, {expr_node}});

      if (!getStr(")")) {
        throw IncorrectParsingException(") was expected after (", __PRETTY_FUNCTION__);
      }
      if (!getStr("lol")) {
        throw IncorrectParsingException("where is lol???", __PRETTY_FUNCTION__);
      }

      Node* condition_met_node = allocator_.init_alloc(Node{LOGIC, CONDITION_MET});
      Node* g_node = getG(func_id);

      while (g_node != nullptr) {
        LOG("getG from If");
        condition_met_node->sons.push_back(g_node);
        g_node = getG(func_id);
      }

      if (!getStr("kek")) {
        throw IncorrectParsingException("where is kek???", __PRETTY_FUNCTION__);
      }

      Node* result_node = allocator_.init_alloc(Node{LOGIC, (oper == "if" ? IF : WHILE),
                                                    {condition_node, condition_met_node}});
      if (oper == "if") {
        Node* else_node = getElse(func_id);

        if (else_node != nullptr) {
          result_node->sons.push_back(else_node);
        }
      }

      return result_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }


  std::pair<std::string, int> getFuncName(bool add_func = false, Node* func_node = nullptr) {
    LOG(tokens_[token_ptr_].value);

    if (done() || tokens_[token_ptr_].token_type == KEYWORD) {
      return {"", -1};
    }

    std::string func_name = tokens_[token_ptr_].value;
    int func_id = tree_.getFunctionId(func_name);

    if ((!add_func && func_id == -1) || !isCorrectVariable(func_name)) {
      return {"", -1};
    }
    ++token_ptr_;

    if (add_func && func_id != -1) {
      throw IncorrectParsingException(std::string("redeclaration of function ") + func_name,
                                      __PRETTY_FUNCTION__);
    }
    if (add_func) {
      func_id = tree_.addFunction(func_name, func_node);
      LOG(std::string("function ") + func_name + std::string(" was added with id ") + std::to_string(func_id));
    }

    return {func_name, func_id};
  }

  Node* getParam(int func_id, bool add_param = false) {
    LOG(tokens_[token_ptr_].value);

    if (done() || tokens_[token_ptr_].token_type == KEYWORD) {
      return nullptr;
    }

    std::string param_name = tokens_[token_ptr_].value;
    int param_id = tree_.getParamId(param_name, func_id);

    if (!isCorrectVariable(param_name) || (!add_param && param_id == -1)) {
      return nullptr;
    }

    ++token_ptr_;

    if (add_param && param_id != -1) {
      throw IncorrectParsingException(std::string("redeclaration of param ") + param_name,
                                      __PRETTY_FUNCTION__);
    }
    if (add_param && tree_.getVariableAddress(param_name, func_id) != -1) {
      throw IncorrectParsingException(std::string("redeclaration of param (a variable has such name) ") + param_name,
                                      __PRETTY_FUNCTION__);
    }

    if (add_param) {
      param_id = tree_.addParam(param_name, func_id);
    }

    return allocator_.init_alloc(Node(PARAM, param_id));
  }

  Node* getFuncHeader(bool add_func = false) {
    Node* func_node = allocator_.init_alloc(Node{USER_FUNCTION, 0});
    std::pair<std::string, int> func_id = getFuncName(add_func, func_node);

    if (func_id.first.empty()) {
      return nullptr;
    }
    func_node->value = func_id.second;


    if (!getStr("(")) {
      throw IncorrectParsingException("( was expected after function name", __PRETTY_FUNCTION__);
    }
    if (add_func) {
      Node* param_node = getParam(func_id.second, add_func);

      while (param_node != nullptr) {
        param_node = getParam(func_id.second, add_func);
      }
    } else {
      Node* e_node = getE(func_id.second);

      while (e_node != nullptr) {
        func_node->sons.push_back(e_node);
        e_node = getE(func_id.second);
      }
    }

    if (!getStr(")")) {
      throw IncorrectParsingException(") was expected after (", __PRETTY_FUNCTION__);
    }
    return func_node;
  }

  Node* getFunc() {
    try {
      if (!getStr("func")) {
        return nullptr;
      }

      LOG(std::string("getFunc ") + std::to_string(token_ptr_));
      Node* func_node = getFuncHeader(true);

      if (func_node == nullptr) {
        throw IncorrectParsingException("function name was expected after func", __PRETTY_FUNCTION__);
      }

      if (!getStr("lol")) {
        throw IncorrectParsingException("where is lol???", __PRETTY_FUNCTION__);
      }

      func_node->sons.push_back(allocator_.init_alloc(Node{VAR_INIT, 0}));

      Node* g_node = getG(func_node->value);
      while (g_node != nullptr) {
        func_node->sons.push_back(g_node);
        g_node = getG(func_node->value);
      }

      if (!getStr("kek")) {
        throw IncorrectParsingException("where is kek???", __PRETTY_FUNCTION__);
      }

      return func_node;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getFuncs() {
    try {
      Node *func_init = allocator_.init_alloc(Node{FUNCS, 0});
      Node *cur_func = getFunc();

      while (cur_func != nullptr) {
        func_init->sons.push_back(cur_func);
        cur_func = getFunc();
      }
      return func_init;
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  Node* getMain() {
    LOG("getMain");

    try {
      if (!getStr("main")) {
        throw IncorrectParsingException("main() was expected",
                                        __PRETTY_FUNCTION__);
      }
      if (!getStr("(")) {
        throw IncorrectParsingException("( was expected after main",
                                        __PRETTY_FUNCTION__);
      }
      if (!getStr(")")) {
        throw IncorrectParsingException(") was expected after (",
                                        __PRETTY_FUNCTION__);
      }

      if (!getStr("lol")) {
        throw IncorrectParsingException("no lol",
                                        __PRETTY_FUNCTION__);
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
    try {
      LOG("getRoot");
      tree_.setRoot(allocator_.init_alloc(Node{ROOT, 0.0, {nullptr, nullptr, nullptr}}));

      tree_.getRoot()->sons[0] = allocator_.init_alloc(Node{VAR_INIT, 0});
      getVarInit(-1);
      tree_.addFunction("main", nullptr);
      tree_.getRoot()->sons[1] = getFuncs();
      tree_.getRoot()->sons[2] = getMain();

      if (token_ptr_ != tokens_.size()) {
        throw IncorrectParsingException(std::string("undefined variable: ") + tokens_[token_ptr_].value,
                                        __PRETTY_FUNCTION__);
      }

      return tree_.getRoot();
    } catch (InterpreterException& exc) {
      throw exc;
    }
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