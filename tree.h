//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_TREE_H
#define DED_PROG_LANG_TREE_H

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <string>
#include <map>
#include <set>
#include <algorithm>

#include "exception.h"
#include "stack_allocator.h"

#define PRINT_STEP(text)\
{\
  if (step_by_step_) {\
    tex_body_ += (text);\
  }\
}

#define PRINT_FORMULA(text)\
{\
  PRINT_STEP("$");\
  PRINT_STEP(text);\
  PRINT_STEP("$");\
}

#define PRINTLN_STEP(text)\
{\
  PRINT_STEP(text);\
  PRINT_STEP("\n\n");\
}

#define PRINTLN_FORMULA(text)\
{\
  PRINT_FORMULA(text);\
  PRINT_STEP("\n\n");\
}

#define PRINT_STEP_NUM()\
{\
  PRINTLN_STEP("$ $");\
  PRINTLN_STEP("\\textbf{Шаг " + std::to_string(step_num_) + "}");\
  ++step_num_;\
}

enum NodeType {
  ROOT,
  FUNCS,
  USER_FUNCTION,
  NUMBER,
  VARIABLE,
  LOCAL_VARIABLE,
  OPERATOR,
  LOGIC,
  MAIN,
  STANDART_FUNCTION,
  VAR_INIT,
  RETURN,
  PARAM
};

enum StandartFunction {
  INPUT = 1,
  OUTPUT,
  SIN,
  COS,
  CALL,
  SQ_ROOT
};

enum LangOperator {
  EQUAL = 1,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  POWER,
  BOOL_EQUAL,
  BOOL_NOT_EQUAL,
  BOOL_LOWER,
  BOOL_GREATER,
  BOOL_NOT_LOWER,
  BOOL_NOT_GREATER,
  BOOL_NOT,
  BOOL_OR,
  BOOL_AND,
  PLUS_EQUAL,
  MINUS_EQUAL,
  MULTIPLY_EQUAL,
  DIVIDE_EQUAL
};

enum LangLogic {
  IF,
  WHILE,
  ELSE,
  CONDITION,
  CONDITION_MET
};

bool isOperator(char ch) {
  return ch == '+' || ch == '-' || ch == '*' || ch == '/';
}

bool isVariable(char ch) {
  return ch >= 'a' && ch <= 'z';
}

bool isCharForDouble(char ch) {
  return isdigit(ch) || ch == '.';
}

size_t operPriority(LangOperator oper_type) {
  if (oper_type == MINUS || oper_type == PLUS) {
    return 1;
  }
  if (oper_type == MULTIPLY || oper_type == DIVIDE) {
    return 2;
  }
  if (oper_type == POWER) {
    return 3;
  }
  throw IncorrectArgumentException("it is not an operator", __PRETTY_FUNCTION__);
}

LangOperator getOperTypeByOper(const std::string& oper) {
  if (oper == "+") {
    return PLUS;
  }
  if (oper == "-") {
    return MINUS;
  }
  if (oper == "*") {
    return MULTIPLY;
  }
  if (oper == "/") {
    return DIVIDE;
  }
  if (oper == "==") {
    return BOOL_EQUAL;
  }
  if (oper == "!=") {
    return BOOL_NOT_EQUAL;
  }
  if (oper == "!") {
    return BOOL_NOT;
  }
  if (oper == "&&") {
    return BOOL_AND;
  }
  if (oper == "||") {
    return BOOL_OR;
  }
  if (oper == "<") {
    return BOOL_LOWER;
  }
  if (oper == ">") {
    return BOOL_GREATER;
  }
  if (oper == ">=") {
    return BOOL_NOT_LOWER;
  }
  if (oper == "<=") {
    return BOOL_NOT_GREATER;
  }
  throw IncorrectArgumentException(std::string("no such operator provided: ") + oper,
                                    __PRETTY_FUNCTION__);
}

char getOperByOperType(LangOperator oper_type) {
  switch (oper_type) {
    case PLUS:
      return '+';
    case MINUS:
      return '-';
    case MULTIPLY:
      return '*';
    case DIVIDE:
      return '/';
    case POWER:
      return '^';
    default:
      throw IncorrectArgumentException(std::string("incorrect node_type: ") + std::to_string(oper_type),
                                       __PRETTY_FUNCTION__);
  }
}

struct Node {
  NodeType type;
  double value{0.0};
  std::vector<Node*> sons;

  bool operator==(const Node& another) const {
    return type == another.type && value == another.value;
  }

  Node(NodeType type, double value):
    type(type), value(value) {

  }

  Node(NodeType type, double value, std::vector<Node*> sons):
    type(type), value(value), sons(sons) {

  }

};

struct FuncBlock {
  Node* func_node;
  std::map<std::string, size_t> param_shift;
  std::map<std::string, size_t> var_shift;
};

class Tree {
 private:
  Node* root_{nullptr};
  std::unordered_map<std::string, size_t> global_var_map_;
  std::unordered_map<std::string, size_t> func_map_;
  std::vector<FuncBlock> func_blocks_;

  mutable size_t cnt_if_{0};
  mutable size_t cnt_while_{0};

 public:
  static StackAllocator<Node> allocator_;

  Tree(Node* node = nullptr): root_(node) {}

  void setRoot(Node* new_root) {
    root_ = new_root;
  }

  Node* getRoot() {
    return root_;
  }

  int getVariableAddress(const std::string& var_name, int func_id) {
    if (func_id == -1) {
      auto iter = global_var_map_.find(var_name);

      if (iter == global_var_map_.end()) {
        return -1;
      }
      return iter->second;
    } else {
      auto iter = func_blocks_[func_id].var_shift.find(var_name);

      if (iter == func_blocks_[func_id].var_shift.end()) {
        return -1;
      }
      return iter->second;
    }
  }

  int addVariable(const std::string& var_name, int func_id, Node* value_node) {
    std::cout << "add variable " << var_name << " to function " << func_id << '\n';

    int result = -1;

    if (func_id == -1) {
      result = global_var_map_.size();

      global_var_map_[var_name] = result;

      root_->sons[0]->sons.push_back(value_node);
      //std::cout << root_->sons.size() << ' ' << root_->sons[0]->sons.size() << '\n';
    } else {
      result = func_blocks_[func_id].var_shift.size();

      func_blocks_[func_id].var_shift[var_name] = result;
      Node* func_node = func_blocks_[func_id].func_node;

      func_node->sons[0]->sons.push_back(value_node);
    }

    return result;
  }

  void initVariable(int func_id, int var_address, Node* value_node) {
    int result = -1;

    std::cout << "try to init " << var_address << " in function " << func_id << '\n';

    if (func_id == -1) {
      root_->sons[0]->sons[var_address] = value_node;
    } else {
      Node* func_node = func_blocks_[func_id].func_node;

      func_node->sons[0]->sons[var_address] = value_node;
    }
  }

  int getFunctionId(const std::string& func_name) {
    auto iter = func_map_.find(func_name);

    if (iter == func_map_.end()) {
      return -1;
    }
    return iter->second;
  }

  int getParamId(const std::string& param_name, int func_id) {
    auto iter = func_blocks_[func_id].param_shift.find(param_name);

    if (iter == func_blocks_[func_id].param_shift.end()) {
      return -1;
    }
    return iter->second;
  }

  int addParam(const std::string& param_name, int func_id) {
    int result = func_blocks_[func_id].param_shift.size();

    func_blocks_[func_id].param_shift[param_name] = result;
    return result;
  }

  int addFunction(const std::string& func_name, Node* func_node) {
    auto iter = func_map_.find(func_name);

    if (iter != func_map_.end()) {
      func_blocks_[iter->second].func_node = func_node;
      return iter->second;
    }
    int result = func_map_.size();

    func_map_[func_name] = result;
    func_blocks_.push_back(FuncBlock());
    func_blocks_.back().func_node = func_node;
    return result;
  }

  void printLevel(const std::string text, FILE* tree_file, size_t level) const {
    for (size_t sep_id = 0; sep_id < level; ++sep_id) {
      fprintf(tree_file, "  ");
    }
    fprintf(tree_file, "%s\n", text.c_str());
  }

  std::string prettyDouble(double value) const {
    if (value == static_cast<int>(value)) {
      return std::to_string(static_cast<int>(value));
    }
    return std::to_string(value);
  }

  void printNodeBegin(Node* node, FILE* tree_file, size_t level) const {
    printLevel(std::string("[ ") + std::to_string(node->type) + " " + prettyDouble(node->value),
               tree_file, level);
  }

  void printNodeEnd(Node* node, FILE* tree_file, size_t level) const {
    printLevel(std::string("]"), tree_file, level);
  }

  void printLeaf(Node* node, FILE* tree_file, size_t level) const {
    printLevel(std::string("[ ") + std::to_string(node->type) + " " + prettyDouble(node->value) + "]",
               tree_file, level);
  }


  void printTreeRec(Node* node, FILE* tree_file, size_t level) const {
    if (node == nullptr) {
      return;
    }
    if (node->sons.size() == 0) {
      printLeaf(node, tree_file, level);
      return;
    }
    printNodeBegin(node, tree_file, level);
    for (size_t child_id = 0; child_id < node->sons.size(); ++child_id) {
      printTreeRec(node->sons[child_id], tree_file, level + 1);
    }
    printNodeEnd(node, tree_file, level);
  }

  void printFuncVars(FILE* tree_file,
                     const std::map<std::string, size_t>& vars) const {
    std::vector<std::pair<size_t, std::string>> sorted_vars;

    for (const std::pair<std::string, size_t>& cp: vars) {
      sorted_vars.push_back({cp.second, cp.first});
    }
    std::sort(sorted_vars.begin(), sorted_vars.end());
    for (const std::pair<size_t, std::string>& cp: sorted_vars) {
      fprintf(tree_file, "%s\n", cp.second.c_str());
    }
  }

  void printFuncs(FILE* tree_file) const {
    fprintf(tree_file, "FUNCS %zu\n", func_blocks_.size());
    std::vector<std::pair<size_t, std::string>> funcs;

    for (const std::pair<std::string, size_t>& cp: func_map_) {
      funcs.push_back({cp.second, cp.first});
    }
    std::sort(funcs.begin(), funcs.end());
    for (const std::pair<size_t, std::string>& cp: funcs) {
      fprintf(tree_file, "%s: PARAMS 0 NEWVAR %zu\n", cp.second.c_str(), func_blocks_[cp.first].var_shift.size());
      printFuncVars(tree_file, func_blocks_[cp.first].var_shift);
    }
  }

  void printVars(FILE* tree_file) const {
    fprintf(tree_file, "VARS %zu\n", global_var_map_.size());
    std::vector<std::pair<size_t, std::string>> vars;

    for (const std::pair<std::string, size_t>& cp: global_var_map_) {
      vars.push_back({cp.second, cp.first});
    }
    std::sort(vars.begin(), vars.end());
    for (const std::pair<size_t, std::string>& cp: vars) {
      fprintf(tree_file, "%s\n", cp.second.c_str());
    }
  }

  void printTree(FILE* tree_file) const {
    printVars(tree_file);
    printFuncs(tree_file);
    printTreeRec(root_, tree_file, 0);
  }

  int getParamCnt(int func_id) const {
    return func_blocks_[func_id].param_shift.size();
  }

  void pushNodeVariable(Node* node, FILE* asm_file, int func_id) const {
    if (node->sons[0]->type == VARIABLE) {
      fprintf(asm_file, "  push [%d]\n", static_cast<int>(node->sons[0]->value));
    } else if (node->sons[0]->type == LOCAL_VARIABLE) {
      fprintf(asm_file, "  push [rcx+%d]\n", static_cast<int>(node->sons[0]->value) + getParamCnt(func_id));
    }
  }

  void popNodeVariable(Node* node, FILE* asm_file, int func_id) const {
    if (node->sons[0]->type == VARIABLE) {
      fprintf(asm_file, "  pop [%d]\n", static_cast<int>(node->sons[0]->value));
    } else if (node->sons[0]->type == LOCAL_VARIABLE) {
      fprintf(asm_file, "  pop [rcx+%d]\n", static_cast<int>(node->sons[0]->value) + getParamCnt(func_id));
    }
  }

  void printAsmRec(Node* node, FILE* asm_file, int func_id) const {
    if (node == nullptr) {
      return;
    }
    //std::cout << "node type " << node->type << '\n';
    switch (node->type) {
      case VAR_INIT:
      {
       // std::cout << "var_init " << func_id << '\n';
        for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
          printAsmRec(node->sons[son_id], asm_file, func_id);
          if (func_id != -1) {
            fprintf(asm_file, "  pop [rcx+%zu]\n", son_id + getParamCnt(func_id));
          } else {
            fprintf(asm_file, "  pop [%zu]\n", son_id);
          }
        }
        return;
      }
      case USER_FUNCTION:
      {
        std::cout << "print user function " << node->value << " from" << func_id << "\n";
        if (func_id == -1) {
          func_id = node->value;
          fprintf(asm_file, ":func_%d\n", static_cast<int>(node->value));
          for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
            printAsmRec(node->sons[son_id], asm_file, node->value);
          }
          fprintf(asm_file, "  ret\n");
        } else {
          fprintf(asm_file, "  call func_%d\n", static_cast<int>(node->value));
        }
        return;
      }
      case NUMBER:
      {
        fprintf(asm_file, "  push %.6lf\n", node->value);
        return;
      }
      case VARIABLE:
      {
        fprintf(asm_file, "  push [%d]\n", static_cast<int>(node->value));
        break;
      }
      case LOCAL_VARIABLE:
      {
        if (func_id != -1) {
          fprintf(asm_file, "  push [rcx+%zu]\n", func_blocks_[func_id].param_shift.size() +
                  static_cast<size_t>(node->value));
        } else {
          fprintf(asm_file, "  push [%d]\n", static_cast<int>(node->value));
        }
        break;
      }
      case PARAM:
      {
        fprintf(asm_file, "  push [rcx+%zu]\n", static_cast<size_t>(node->value));
        break;
      }
      case OPERATOR:
      {
        int oper_type = static_cast<int>(node->value);

        if (oper_type != PLUS_EQUAL && oper_type != MINUS_EQUAL
            && oper_type != MULTIPLY_EQUAL && oper_type != DIVIDE_EQUAL) {
          for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
            printAsmRec(node->sons[son_id], asm_file, func_id);
          }
        }

        switch (oper_type) {
          case EQUAL:
            popNodeVariable(node, asm_file, func_id);
            break;
          case PLUS_EQUAL:
            pushNodeVariable(node, asm_file, func_id);
            printAsmRec(node->sons[1], asm_file, func_id);
            fprintf(asm_file, "  add\n");
            popNodeVariable(node, asm_file, func_id);
            break;
          case MINUS_EQUAL:
            pushNodeVariable(node, asm_file, func_id);
            printAsmRec(node->sons[1], asm_file, func_id);
            fprintf(asm_file, "  sub\n");
            popNodeVariable(node, asm_file, func_id);
            break;
          case MULTIPLY_EQUAL:
            pushNodeVariable(node, asm_file, func_id);
            printAsmRec(node->sons[1], asm_file, func_id);
            fprintf(asm_file, "  mul\n");
            popNodeVariable(node, asm_file, func_id);
            break;
          case DIVIDE_EQUAL:
            pushNodeVariable(node, asm_file, func_id);
            printAsmRec(node->sons[1], asm_file, func_id);
            fprintf(asm_file, "  div\n");
            popNodeVariable(node, asm_file, func_id);
            break;
          case PLUS:
            fprintf(asm_file, "  add\n");
            break;
          case MINUS:
            fprintf(asm_file, "  sub\n");
            break;
          case MULTIPLY:
            fprintf(asm_file, "  mul\n");
            break;
          case DIVIDE:
            fprintf(asm_file, "  div\n");
            break;
          case POWER:
            fprintf(asm_file, "  power\n");
            break;
          case BOOL_EQUAL:
            fprintf(asm_file, "  is_equal\n");
            break;
          case BOOL_NOT_EQUAL:
            fprintf(asm_file, "  is_nequal\n");
            break;
          case BOOL_AND:
            fprintf(asm_file, "  and\n");
            break;
          case BOOL_OR:
            fprintf(asm_file, "  or\n");
            break;
          case BOOL_GREATER:
            fprintf(asm_file, "  greater\n");
            break;
          case BOOL_LOWER:
            fprintf(asm_file, "  lower\n");
            break;
          case BOOL_NOT_GREATER:
            fprintf(asm_file, "  ngreater\n");
            break;
          case BOOL_NOT_LOWER:
            fprintf(asm_file, "  nlower\n");
            break;
          default:
            throw IncorrectArgumentException(std::string("no such operator ") + std::to_string(node->value),
                                             __PRETTY_FUNCTION__);
        }
        break;
      }
      case STANDART_FUNCTION:
      {
        if (node->value != CALL) {
          for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
            printAsmRec(node->sons[son_id], asm_file, func_id);
          }
        }
        int std_func_type = static_cast<int>(node->value);

        switch (std_func_type) {
          case INPUT:
            fprintf(asm_file, "  in rax\n");
            fprintf(asm_file, "  push rax\n");
            if (node->sons[0]->type == VARIABLE) {
              fprintf(asm_file, "  pop [%d]\n", static_cast<int>(node->sons[0]->value));
            } else if (node->sons[0]->type == LOCAL_VARIABLE) {
              fprintf(asm_file, "  pop [rcx+%d]\n", static_cast<int>(node->sons[0]->value));
            }
            break;
          case OUTPUT:
            fprintf(asm_file, "  pop rbx\n");
            fprintf(asm_file, "  out rbx\n");
            break;
          case SIN:
            fprintf(asm_file, "  sin\n");
            break;
          case COS:
            fprintf(asm_file, "  cos\n");
            break;
          case SQ_ROOT:
            fprintf(asm_file, "  sqrt\n");
            break;
          case CALL:
            fprintf(asm_file, "  push rcx\n");
            fprintf(asm_file, "  push %zu\n", func_blocks_[func_id].param_shift.size() +
                                              func_blocks_[func_id].var_shift.size());
            fprintf(asm_file, "  add\n");
            fprintf(asm_file, "  pop rcx\n");

            fprintf(asm_file, "  call func_%d\n", static_cast<int>(node->sons[0]->value));

            fprintf(asm_file, "  push rcx\n");
            fprintf(asm_file, "  push %zu\n", func_blocks_[func_id].param_shift.size() +
                                              func_blocks_[func_id].var_shift.size());
            fprintf(asm_file, "  sub\n");
            fprintf(asm_file, "  pop rcx\n");
            break;
          default:
            throw IncorrectArgumentException(std::string("no such operator ") + std::to_string(node->value),
                                             __PRETTY_FUNCTION__);
        }
        break;
      }
      case MAIN:
      {
        fprintf(asm_file, "\n:func_0\n");
        for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
          printAsmRec(node->sons[son_id], asm_file, 0);
        }
        fprintf(asm_file, "  end\n");
        break;
      }
      case FUNCS:
      {
        std::cout << "user func count " << node->sons.size() << "\n";
        for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
          std::cout << "declare func " << node->sons[son_id]->value << " from " << func_id << "\n";
          printAsmRec(node->sons[son_id], asm_file, func_id);
        }
        break;
      }
      case RETURN:
      {
        if (node->sons.size() == 1) {
          printAsmRec(node->sons[0], asm_file, func_id);
        }
        if (func_id != 0) {
          fprintf(asm_file, "  ret\n");
        } else {
          fprintf(asm_file, "  end\n");
        }
        break;
      }
      case ROOT:
      {
        printAsmRec(node->sons[0], asm_file, func_id);
        fprintf(asm_file, "  jmp func_0\n");
        printAsmRec(node->sons[1], asm_file, func_id);
        printAsmRec(node->sons[2], asm_file, func_id);
        break;
      }
      case LOGIC:
      {
        int logic_type = static_cast<int>(node->value);

        switch (logic_type) {
          case IF:
            printAsmRec(node->sons[0], asm_file, func_id);
            fprintf(asm_file, "  push 0\n");
            fprintf(asm_file, "  je if_end_%zu\n", cnt_if_);
            printAsmRec(node->sons[1], asm_file, func_id);
            fprintf(asm_file, "  jmp if_block_end_%zu\n", cnt_if_);
            fprintf(asm_file, "  :if_end_%zu\n", cnt_if_);
            if (node->sons.size() > 2) {
              printAsmRec(node->sons[2], asm_file, func_id);
            }
            fprintf(asm_file, "  :if_block_end_%zu\n", cnt_if_);
            ++cnt_if_;
            break;
          case ELSE:
            for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
              printAsmRec(node->sons[son_id], asm_file, func_id);
            }
            break;
          case WHILE:
            fprintf(asm_file, "  :while_begin_%zu\n", cnt_while_);
            printAsmRec(node->sons[0], asm_file, func_id);
            fprintf(asm_file, "  push 0\n");
            fprintf(asm_file, "  je while_end_%zu\n", cnt_while_);
            printAsmRec(node->sons[1], asm_file, func_id);
            fprintf(asm_file, "  jmp while_begin_%zu\n", cnt_while_);
            fprintf(asm_file, "  :while_end_%zu\n", cnt_while_);
            ++cnt_while_;
            break;
          case CONDITION:
            printAsmRec(node->sons[0], asm_file, func_id);
            break;
          case CONDITION_MET:
          {
            for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
              printAsmRec(node->sons[son_id], asm_file, func_id);
            }
            break;
          }
          default:
            throw IncorrectArgumentException(std::string("no such separate logic block was provided: ")
                                               + std::to_string(logic_type), __PRETTY_FUNCTION__);
        }
        break;
      }
      default:
      {
        throw IncorrectArgumentException(std::string("no such node type:") + std::to_string(node->type),
                                         __PRETTY_FUNCTION__);
      }
    }
  }

  void printAssembler(FILE* asm_file) const {
    std::cout << "print asm rec\n";
    printAsmRec(root_, asm_file, -1);
  }
};

#endif //DED_PROG_LANG_TREE_H
