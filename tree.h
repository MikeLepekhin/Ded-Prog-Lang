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
  REAL_NUMBER,
  OPER_PLUS,
  OPER_MINUS,
  OPER_MUL,
  OPER_DIV,
  OPER_SQRT,
  OPER_EXP,
  OPER_SIN,
  OPER_COS,
  OPER_POWER,
  VARIABLE,
  SCAN_NODE,
  PRINT_NODE,
  FUNC_BODY_NODE,
  FUNC_NODE,
  ASSIGN_NODE,
  V_NODE
};

bool isOperator(char ch) {
  return ch == '+' || ch == '-' || ch == '*' || ch == '/';
}

bool isOperNode(NodeType oper_type) {
  return oper_type > 0 && oper_type <= OPER_POWER;
}

bool isVariable(char ch) {
  return ch >= 'a' && ch <= 'z';
}

bool isCharForDouble(char ch) {
  return isdigit(ch) || ch == '.';
}

size_t operPriority(NodeType oper_type) {
  if (oper_type == OPER_MINUS || oper_type == OPER_PLUS) {
    return 1;
  }
  if (oper_type == OPER_MUL || oper_type == OPER_DIV) {
    return 2;
  }
  throw IncorrectArgumentException("it is not an operator", __PRETTY_FUNCTION__);
}

NodeType getNodeTypeByOper(char oper) {
  switch (oper) {
    case '+':
      return OPER_PLUS;
    case '-':
      return OPER_MINUS;
    case '*':
      return OPER_MUL;
    case '/':
      return OPER_DIV;
    default:
      throw IncorrectArgumentException(std::string("no such operator provided: ") + oper,
                                       __PRETTY_FUNCTION__);
  }
}

char getOperByNodeType(NodeType node_type) {
  switch (node_type) {
    case OPER_PLUS:
      return '+';
    case OPER_MINUS:
      return '-';
    case OPER_MUL:
      return '*';
    case OPER_DIV:
      return '/';
    default:
      throw IncorrectArgumentException(std::string("incorrect node_type: ") + std::to_string(node_type),
                                       __PRETTY_FUNCTION__);
  }
}

struct VariableBlock {
  std::string var_name;
  size_t ram_location;
};

struct FuncBlock {
  std::string func_name;
  size_t ram_location;
  std::map<std::string, size_t> var_shift;
};

struct Node {
  NodeType type;
  double result{0.0};
  Node* left_son{nullptr};
  Node* right_son{nullptr};
  void* block{nullptr};

  static StackAllocator<VariableBlock> var_allocator_;
  static StackAllocator<FuncBlock> func_allocator_;

  Node(NodeType type): type(type) {}

  Node(NodeType type, double result):
    type(type), result(result) {}

  Node(NodeType type, const std::string& var_name):
      type(type) {
    if (type != VARIABLE) {
      throw IncorrectArgumentException("only variables should have such constructor",
                                       __PRETTY_FUNCTION__);
    }
    block = var_allocator_.init_alloc(VariableBlock{var_name, 0});
    LOG("variable node was constructed");
  }

  Node(NodeType type, Node* left_son, Node* right_son):
    type(type), left_son(left_son), right_son(right_son) {
    if ((type == REAL_NUMBER || type == VARIABLE) &&
      (left_son != nullptr || right_son != nullptr)) {
      throw IncorrectArgumentException("node of such type should be a leaf",
                                       __PRETTY_FUNCTION__);
    }
  }

  Node(NodeType type, Node* left_son):
    type(type), left_son(left_son) {
    if (type != SCAN_NODE && type != PRINT_NODE) {
      throw IncorrectArgumentException("node of such type should have not 1 son",
                                       __PRETTY_FUNCTION__);
    }
  }

  Node(NodeType type, const std::string var_name, Node* expr_node):
      type(type), left_son(expr_node), right_son(nullptr) {
    if (type != ASSIGN_NODE && type != V_NODE && type != FUNC_NODE) {
      throw IncorrectArgumentException("such constructor is only available for ASSIGN nodes"
                                         " and FUNC nodes");
    }
    if (type == ASSIGN_NODE || type == V_NODE) {
      block = reinterpret_cast<void*>(var_allocator_.init_alloc(VariableBlock{var_name, 0}));
    } else {
      block = reinterpret_cast<void*>(func_allocator_.init_alloc(FuncBlock{var_name, 0}));
    }
  }

  void setSons(Node* L, Node* R) {
    left_son = L;
    right_son = R;
  }

  std::string getVarName() const {
    return reinterpret_cast<VariableBlock*>(block)->var_name;
  }

  std::string getFuncName() const {
    return reinterpret_cast<FuncBlock*>(block)->func_name;
  }

  void setRAM(size_t address) const {
    reinterpret_cast<FuncBlock*>(block)->ram_location = address;
  }

  size_t getRAM() const {
    return reinterpret_cast<FuncBlock*>(block)->ram_location;
  }

  std::string to_str() const {
    switch (type) {
      case OPER_MINUS:
        return "-";
      case OPER_PLUS:
        return "+";
      case OPER_MUL:
        return "*";
      case OPER_DIV:
        return "/";
      case VARIABLE:
        return getVarName();
      case REAL_NUMBER:
        return std::to_string(result);
      default:
        throw IncorrectArgumentException("incorrect type of node",
                                         __PRETTY_FUNCTION__);
    }
  }

  bool operator==(const Node& another) const {
    return type == another.type && result == another.result
      && getVarName() == another.getVarName();
  }
};

class Tree {
 private:
  Node* root_{nullptr};
  FILE* step_by_step_{nullptr};
  size_t step_num_{0};
  std::unordered_map<std::string, size_t> func_map_;
  mutable std::string tex_begin_;
  mutable std::string tex_body_;
  mutable std::string tex_end_;

  void printProgramLevel(const std::string& str, size_t level) const {
    for (size_t char_id = 0; char_id < level; ++char_id) {
      std::cout << '-';
    }
    std::cout << str << '\n';
  }

  double parseDouble(const std::string& str, size_t* pos) {
    size_t cur_pos = *pos;

    while (cur_pos + 1 < str.size() && isCharForDouble(str[cur_pos + 1])) {
      ++cur_pos;
    }
    double res_value = atof(str.substr(*pos, cur_pos - *pos + 1).c_str());
    *pos = cur_pos;
    return res_value;
  }

  double parseDoubleRev(const std::string& str, int* pos) {
    int cur_pos = *pos;

    while (cur_pos >= 1 && isCharForDouble(str[cur_pos - 1])) {
      --cur_pos;
    }
    double res_value = atof(str.substr(cur_pos, *pos - cur_pos + 1).c_str());
    *pos = cur_pos;
    return res_value;
  }

  Node* getParentNode(Node* left_son, Node* right_son, NodeType node_type) {
    if (node_type == VARIABLE || node_type == REAL_NUMBER) {
      throw IncorrectParsingException("a node that is not an operator can't be a parent",
                                      __PRETTY_FUNCTION__);
    }

    Node* result_node = allocator_.init_alloc(Node{node_type, left_son, right_son});
    return result_node;
  }

  Node* calcNode(Node* left_son, Node* right_son, NodeType node_type) {
    double left_value = left_son->result;
    double right_value = right_son->result;

    switch (node_type) {
      case OPER_PLUS:
        return allocator_.init_alloc(Node{REAL_NUMBER, left_value + right_value});
      case OPER_MINUS:
        return allocator_.init_alloc(Node{REAL_NUMBER, left_value - right_value});
      case OPER_MUL:
        return allocator_.init_alloc(Node{REAL_NUMBER, left_value * right_value});
      case OPER_DIV:
        if (right_value == 0.0) {
          throw DivisionByZeroException("", __PRETTY_FUNCTION__);
        }
        return allocator_.init_alloc(Node{REAL_NUMBER, left_value / right_value});
      default:
        throw IncorrectArgumentException("an operator was expected", __PRETTY_FUNCTION__);
    }
  }

  void makeNiceVariable(Node* node, Node* left_son, Node* right_son, bool is_swapped = false) {

    if ((node->type == OPER_PLUS || node->type == OPER_MINUS) &&
      right_son->result == 0.0) {
      *node = *left_son;
      node->left_son = node->right_son = nullptr;
      return;
    }
    if (node->type == OPER_MUL && right_son->result == 1.0) {
      *node = *left_son;
      node->left_son = node->right_son = nullptr;
      return;
    }
    if (node->type == OPER_MUL && right_son->result == 1.0) {
      *node = Node(REAL_NUMBER, 0.0);
      node->left_son = node->right_son = nullptr;
      return;
    }
    if (!is_swapped && node->type == OPER_DIV && right_son->result == 0.0) {
      *node = *left_son;
      node->left_son = node->right_son = nullptr;
    } else if (is_swapped && node->type == OPER_DIV && left_son->result == 0.0) {
      *node = *right_son;
      node->left_son = node->right_son = nullptr;
    }
  }

  void processOpers(std::vector<char>* was_opers, std::vector<Node*>* was_operands) {
    std::vector<char>& opers = *was_opers;
    std::vector<Node*>& operands = *was_operands;

    while (!opers.empty() && operands.size() >= 2) {
      Node* operand_b = operands.back(); operands.pop_back();
      Node* operand_a = operands.back(); operands.pop_back();
      char oper = opers.back(); opers.pop_back();

      operands.push_back(getParentNode(operand_b, operand_a,
                                       getNodeTypeByOper(oper)));
    }
  }

  void printPrefixRec(Node* cur_node) {
    if (cur_node == nullptr) {
      return;
    }

    switch (cur_node->type) {
      case VARIABLE:
        std::cout << cur_node->getVarName() << ' ';
        break;
      case REAL_NUMBER:
        std::cout << cur_node->result << ' ';
        break;
      case OPER_PLUS:
        std::cout << '+' << ' ';
        break;
      case OPER_MINUS:
        std::cout << '-' << ' ';
        break;
      case OPER_MUL:
        std::cout << '*' << ' ';
        break;
      case OPER_DIV:
        std::cout << '/' << ' ';
        break;
    }
    printPrefixRec(cur_node->left_son);
    printPrefixRec(cur_node->right_son);
  }

  void getInfixSubtree(Node* parent_root, Node* subtree_root, std::string* infix_notation) {
    if (!subtree_root) {
      return;
    }
    if (subtree_root->type != REAL_NUMBER && subtree_root->type != VARIABLE &&
      operPriority(parent_root->type) > operPriority(subtree_root->type)) {
      infix_notation->push_back('(');
    }
    getInfixRec(subtree_root, infix_notation);
    if (subtree_root->type != REAL_NUMBER && subtree_root->type != VARIABLE &&
      operPriority(parent_root->type) > operPriority(subtree_root->type)) {
      infix_notation->push_back(')');
    }
  }

  void getInfixRec(Node* cur_node, std::string* infix_notation) {
    if (cur_node == nullptr) {
      return;
    }

    getInfixSubtree(cur_node, cur_node->left_son, infix_notation);
    *infix_notation += cur_node->to_str();
    getInfixSubtree(cur_node, cur_node->right_son, infix_notation);
  }

  Tree recCopy(Node* node) const {
    if (!node) {
      return nullptr;
    }

    Node* result = allocator_.init_alloc(*node);
    Tree left_tree = recCopy(node->left_son);
    Tree right_tree = recCopy(node->right_son);
    result->setSons(left_tree.root_, right_tree.root_);

    return Tree(result);
  }

  void simplify(Node*& node) {
    if (node == nullptr) {
      return;
    }

    simplify(node->left_son);
    simplify(node->right_son);
    if (node->left_son != nullptr && node->right_son != nullptr) {
      if (node->left_son->type == REAL_NUMBER && node->right_son->type == REAL_NUMBER) {
        node = calcNode(node->left_son, node->right_son, node->type);
      } else if (node->left_son->type == VARIABLE && node->right_son->type == REAL_NUMBER) {
        makeNiceVariable(node, node->left_son, node->right_son);
      } else if (node->right_son->type == VARIABLE && node->left_son->type == REAL_NUMBER) {
        makeNiceVariable(node, node->right_son, node->left_son, true);
      } else if (node->type == OPER_MINUS &&
        *(node->left_son) == *(node->right_son)) {
        node = allocator_.init_alloc(Node(REAL_NUMBER, 0.0));
      } else if ((node->type == OPER_PLUS || node->type == OPER_MINUS) &&
        node->right_son->type == REAL_NUMBER && node->right_son->result == 0.0) {
        node = node->left_son;
      } else if ((node->type == OPER_PLUS || node->type == OPER_MINUS) &&
        node->left_son->type == REAL_NUMBER && node->left_son->result == 0.0) {
        node = node->right_son;
      }
    }
  }

  Tree differentiateRec(Node* node, char variable) {
    if (node == nullptr) {;
      return Tree(node);
    }

    if (node->type == REAL_NUMBER || (node->type == VARIABLE && node->getVarName()[0] != variable)) {
      PRINT_STEP_NUM();
      PRINTLN_STEP("В это трудно поверить, но\n");
      PRINT_STEP(node->to_str());
      PRINTLN_STEP("'=0\n");

      return Tree(allocator_.init_alloc(Node(REAL_NUMBER, 0.0)));
    } else if (node->type == VARIABLE && node->getVarName()[0] == variable) {
      PRINT_STEP_NUM();
      PRINTLN_STEP("Если бы вы знали, что такое дифференцирование, то сами могли бы получить, что\n");
      PRINT_STEP(node->to_str());
      PRINTLN_STEP("'=1\n");
      return Tree(allocator_.init_alloc(Node(REAL_NUMBER, 1.0)));
    }

    std::string left_infix = Tree(node->left_son).getInfixNotation();
    std::string right_infix = Tree(node->right_son).getInfixNotation();
    Tree left_diff = differentiateRec(node->left_son, variable).root_;
    Tree right_diff = differentiateRec(node->right_son, variable).root_;

    std::string left_diff_infix = left_diff.getInfixNotation();
    std::string right_diff_infix = right_diff.getInfixNotation();


    switch (node->type) {
      case OPER_PLUS:
      {
        Tree result = left_diff + right_diff;

        PRINT_STEP_NUM();
        PRINTLN_STEP("Это было не просто, но мы посчитали значения производных $" +
          left_infix + "$ и $" + right_infix + "$");
        PRINTLN_STEP("Производную суммы будем считать по формуле:");
        PRINTLN_STEP("$(u+v)' = u' + v'$\n");
        PRINT_STEP("Получим:\n");
        PRINTLN_FORMULA("(" + left_infix + " + " + right_infix + ")' = " + left_diff_infix + "' + " + right_diff_infix + "' =");
        PRINTLN_FORMULA(result.getInfixNotation());

        return result;
      }
      case OPER_MINUS:
      {
        PRINT_STEP_NUM();
        PRINTLN_STEP("Методом пристального взгляда были посчитаны производные $" +
          left_infix + "$ и $" + right_infix + "$");
        PRINTLN_STEP("Производную разности будем считать по формуле (которую вы, к сожалению, не знаете):");
        PRINTLN_STEP("$(u-v)' = u' - v'$\n");

        Tree result = left_diff - right_diff;

        PRINT_STEP("Получим:\n");
        PRINTLN_FORMULA("(" + left_infix + " - " + right_infix + ")' = " + left_diff_infix + "' - " + right_diff_infix + "' =");
        PRINTLN_FORMULA(result.getInfixNotation());

        return left_diff - right_diff;
      }
      case OPER_MUL:
      {
        PRINT_STEP_NUM();
        PRINTLN_STEP("Каким-то чудесным образом были найдены производные $" +
          left_infix + "$ и $" + right_infix + "$");
        PRINTLN_STEP("Производную произведения будем считать по известной всем формуле:");
        PRINTLN_STEP("$(u*v)' = u'v + v'u$\n");

        Tree left_subtree = Tree(node->left_son).copy();
        Tree right_subtree = Tree(node->right_son).copy();
        Tree result = left_diff * right_subtree + left_subtree * right_diff;

        PRINT_STEP("Получим:\n");
        PRINTLN_FORMULA("(" + left_infix + " * " + right_infix + ")' = ");
        PRINTLN_FORMULA(left_diff_infix + "*" + right_infix + " + " + right_diff_infix + "*" + left_infix + "' =");
        PRINTLN_FORMULA(result.getInfixNotation());

        return result;
      }
      case OPER_DIV:
      {
        PRINT_STEP_NUM();
        PRINTLN_STEP("Путём мучительных усилий мы продифференцировали $" +
          left_infix + "$ и $" + right_infix + "$");
        PRINTLN_STEP("Производную частного будем считать вот так:");
        PRINTLN_STEP("$(\\frac{u}{v})' = \\frac{u'v - v'u}{v^2}$\n");

        Tree left_subtree = Tree(node->left_son).copy();
        Tree right_subtree = Tree(node->right_son).copy();

        Tree numerator = left_diff * right_subtree - left_subtree * right_diff;
        Tree denominator = right_subtree * right_subtree;
        Tree result = numerator / denominator;

        PRINT_STEP("Получим:\n");
        PRINTLN_FORMULA("(" + left_infix + " / " + right_infix + ")' = ");
        PRINTLN_FORMULA(result.getInfixNotation());
        return result;
      }
      default:
        throw IncorrectParsingException("unknown node type", __PRETTY_FUNCTION__);

    }
  }

  Tree applyOper(const Tree& another, NodeType oper_type) const {
    PRINT_STEP(std::string("Применяю оператор ") + getOperByNodeType(oper_type) + std::string(" к "));

    Tree left_son = this->copy();
    Tree right_son = another.copy();

    return Tree(allocator_.init_alloc(Node(oper_type, left_son.root_, right_son.root_)));
  }

  void fillVarMapsRec(Node* node, std::map<std::string, size_t>* var_map = nullptr) {
    try {
      if (node == nullptr) {
        return;
      }
      if (node->type == FUNC_NODE) {
        var_map = &reinterpret_cast<FuncBlock *>(node->block)->var_shift;
      } else if (node->type == VARIABLE || node->type == ASSIGN_NODE) {
        if (var_map == nullptr || var_map->find(node->getVarName()) == var_map->end()) {
          throw UndefinedVariableException("undefined variable: " + node->getVarName(),
                                           __PRETTY_FUNCTION__);
        }
        node->setRAM((*var_map)[node->getVarName()]);
        /*std::cout << std::string("[") + std::to_string((*var_map)[node->getVarName()])
          + "]=" + node->getVarName() << '\n';*/
      } else if (node->type == V_NODE) {
        if (var_map->find(node->getVarName()) != var_map->end()) {
          throw RedefinedVariableException("redefinition of variable: " + node->getVarName(),
                                           __PRETTY_FUNCTION__);
        }
        node->setRAM(var_map->size());
        (*var_map)[node->getVarName()] = var_map->size();
        //std::cout << "add variable " << node->getVarName() << "\n";
      }

      fillVarMapsRec(node->left_son, var_map);
      fillVarMapsRec(node->right_son, var_map);
      if (node->type == FUNC_NODE) {
        std::cout << "variables of function " + node->getFuncName() + ":\n";
        for (const std::pair<std::string, size_t> &func_var: *var_map) {
          std::cout << func_var.first << ' ' << func_var.second << '\n';
        }
        std::cout << "\n";
      }
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }

  void translateRec(Node* node, FILE* asm_result) {
    if (node == nullptr) {
      return;
    }
    if (node->type == REAL_NUMBER) {
      fprintf(asm_result, "  push %lf\n", node->result);
    } else if (isOperNode(node->type)) {
      translateRec(node->left_son, asm_result);
      translateRec(node->right_son, asm_result);

      switch (node->type) {
        case OPER_PLUS:
          fprintf(asm_result, "  add\n");
          break;
        case OPER_MINUS:
          fprintf(asm_result, "  sub\n");
          break;
        case OPER_MUL:
          fprintf(asm_result, "  mul\n");
          break;
        case OPER_DIV:
          fprintf(asm_result, "  div\n");
          break;
        case OPER_SQRT:
          fprintf(asm_result, "  sqrt\n");
          break;
        case OPER_EXP:
          fprintf(asm_result, "  exp\n");
          break;
        case OPER_SIN:
          fprintf(asm_result, "  sin\n");
          break;
        case OPER_COS:
          fprintf(asm_result, "  cos\n");
          break;
        case OPER_POWER:
          fprintf(asm_result, "  power\n");
          break;
        default:
          break;
      }
      return;
    } else if (node->type == FUNC_NODE) {
      fprintf(asm_result, "\n:func_%s\n", node->getFuncName().c_str());
      translateRec(node->left_son, asm_result);
      if (node->getFuncName() == "main") {
        fprintf(asm_result, "  end\n");
      } else {
        fprintf(asm_result, "  ret\n");
      }
    } else if (node->type == V_NODE || node->type == ASSIGN_NODE) {
      if (node->left_son != nullptr) {
        translateRec(node->left_son, asm_result);
        fprintf(asm_result, "  pop [rcx+%zu]\n", node->getRAM());
      } else {
        fprintf(asm_result, "  push 0\n");
        fprintf(asm_result, "  pop [rcx+%zu]\n", node->getRAM());
      }
    } else if (node->type == VARIABLE) {
      fprintf(asm_result, "  push [rcx+%zu]\n", node->getRAM());
    } else if (node->type == FUNC_BODY_NODE) {
      translateRec(node->left_son, asm_result);
      translateRec(node->right_son, asm_result);
    } else if (node->type == SCAN_NODE) {
      fprintf(asm_result, "  in rax\n");
      fprintf(asm_result, "  push rax\n");
      fprintf(asm_result, "  pop [rcx+%zu]\n", node->left_son->getRAM());
    } else if (node->type == PRINT_NODE) {
      translateRec(node->left_son, asm_result);
      fprintf(asm_result, "  pop rbx\n");
      fprintf(asm_result, "  out rbx\n");
    } else {
      throw IncorrectParsingException("dsdsdasd", __PRETTY_FUNCTION__);
    }
  }

 public:
  static StackAllocator<Node> allocator_;

  Tree(Node* root_node): root_(root_node) {}

  Tree(const std::string& polish_notation) {
    //std::cout << "polish in " << polish_notation << '\n';

    std::vector<char> opers;
    std::vector<Node*> operands;

    for (int char_id = polish_notation.size() - 1; char_id >= 0; --char_id) {
      char cur_ch = polish_notation[char_id];

      if (std::isspace(cur_ch)) {
        continue;
      }
      if (isOperator(cur_ch)) {
        opers.push_back(cur_ch);
        processOpers(&opers, &operands);
      } else if (isCharForDouble(cur_ch)) {
        double value = parseDoubleRev(polish_notation, &char_id);

        operands.push_back(allocator_.init_alloc(Node(REAL_NUMBER, value)));
      } else if (isVariable(cur_ch)) {
        operands.push_back(allocator_.init_alloc(Node(VARIABLE, std::string("") + cur_ch)));
      } else {
        throw IncorrectArgumentException(std::string("incorrect operator or variable: ") + cur_ch,
                                         __PRETTY_FUNCTION__);
      }
    }

    if (operands.size() != 1) {
      throw IncorrectParsingException("invalid number of tree roots: " + std::to_string(operands.size()),
                                      __PRETTY_FUNCTION__);
    }

    root_ = operands.back();
    simplify(root_);
  }

  Tree copy() const {
    Tree result(nullptr);

    return recCopy(root_);
  }

  std::string getInfixNotation() {
    std::string result = "";
    getInfixRec(root_, &result);

    return result;
  }

  void printPrefixNotation() {
    printPrefixRec(root_);
    std::cout << '\n';
  }

  void printInfixNotation() {
    std::cout << getInfixNotation() << '\n';
  }

  Tree derivative(char variable = 'x', FILE* step_by_step = nullptr) {
    step_by_step_ = step_by_step;

    tex_begin_ = "\\documentclass[12pt]{article}\n"
      "\\usepackage[utf8]{inputenc}\n"
      "\\usepackage[russian]{babel}\n"
      "\\usepackage{hyperref}\n"
      "\\usepackage{color}\n"
      "\\usepackage{amssymb}\n"
      "\\usepackage{amsmath}\n"
      "\\usepackage{graphicx}\n"
      "\\usepackage{tikz}\n"
      "\n"
      "\\author{Differentiator}\n"
      "\\title{Подсчёт производной функции " + getInfixNotation() +
      "}\n"
        "\n"
        "\\begin{document}\n"
        "\\maketitle\n";
    tex_body_ = "";
    tex_end_ = "\\end{document}";

    PRINTLN_STEP("Диффернём немножечко.\n");



    Tree der_tree = differentiateRec(root_, variable);

    der_tree.simplify(der_tree.root_);

    PRINTLN_STEP("$ $");
    PRINT_STEP("Производная, которую мы так долго считали:\n");
    PRINTLN_STEP(der_tree.getInfixNotation());
    PRINTLN_STEP("$ $");
    PRINT_STEP("На этом процесс дифференцирования закончен.\n");
    PRINT_STEP("Надеемся, что вы знаете все использованные в нём слова.\n");
    fprintf(step_by_step, "%s", tex_begin_.c_str());
    fprintf(step_by_step, "%s", tex_body_.c_str());
    fprintf(step_by_step, "%s", tex_end_.c_str());
    return der_tree;
  }

  Tree operator+(const Tree& another) const {
    return applyOper(another, OPER_PLUS);
  }

  Tree operator-(const Tree& another) const {
    return applyOper(another, OPER_MINUS);
  }

  Tree operator*(const Tree& another) const {
    return applyOper(another, OPER_MUL);
  }

  Tree operator/(const Tree& another) const {
    return applyOper(another, OPER_DIV);
  }

  void printProgramRec(Node* node, size_t level) const {
    if (node == nullptr) {
      return;
    }

    switch (node->type) {
      case FUNC_NODE:
        printProgramLevel("func " + node->getFuncName(), level);
        break;
      case V_NODE:
        printProgramLevel("var " + node->getVarName(), level);
        break;
      case ASSIGN_NODE:
        printProgramLevel(node->getVarName() + "=", level);
        break;
      case OPER_PLUS:
        printProgramLevel("+", level);
        break;
      case OPER_MINUS:
        printProgramLevel("-", level);
        break;
      case OPER_MUL:
        printProgramLevel("*", level);
        break;
      case OPER_DIV:
        printProgramLevel("/", level);
        break;
      case FUNC_BODY_NODE:
        //printProgramLevel("", level);
        break;
      case VARIABLE:
        printProgramLevel(node->getVarName(), level);
        break;
      case SCAN_NODE:
        printProgramLevel("scan(" + node->left_son->getVarName() + ")", level);
        break;
      case PRINT_NODE:
        printProgramLevel("print:", level);
        break;
      case REAL_NUMBER:
        printProgramLevel(std::to_string(node->result), level);
        break;
      default:
        throw IncorrectArgumentException("no such node type: " + std::to_string(node->type));
    }
    if (node->type == SCAN_NODE) {
      return;
    }
    printProgramRec(node->left_son, level + (node->type != FUNC_BODY_NODE));
    printProgramRec(node->right_son, level + (node->type != FUNC_BODY_NODE));
  }

  void printProgram() const {
    std::cout << "program tree:\n";
    printProgramRec(root_, 0);
  }

  void translateToAsm(FILE* asm_result) {
    fillVarMapsRec(root_);
    fprintf(asm_result, "jmp func_main\n");
    translateRec(root_, asm_result);
  }
};

#endif //DED_PROG_LANG_TREE_H
