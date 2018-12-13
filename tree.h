//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_TREE_H
#define DED_PROG_LANG_TREE_H

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>

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
  VARIABLE
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

struct Node {
  NodeType type;
  double result{0.0};
  char var_name{'0'};

  Node* left_son{nullptr};
  Node* right_son{nullptr};

  Node(NodeType type): type(type) {}

  Node(NodeType type, double result):
    type(type), result(result) {}

  Node(NodeType type, double result, char var_name):
    type(type), result(result), var_name(var_name) {}

  Node(NodeType type, Node* left_son, Node* right_son):
    type(type), left_son(left_son), right_son(right_son) {
    if ((type == REAL_NUMBER || type == VARIABLE) &&
      (left_son != nullptr || right_son != nullptr)) {
      throw IncorrectArgumentException("node of such type should be a leaf",
                                       __PRETTY_FUNCTION__);
    }
  }

  void setSons(Node* L, Node* R) {
    left_son = L;
    right_son = R;
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
        return std::string("") + var_name;
      case REAL_NUMBER:
        return std::to_string(result);
      default:
        throw IncorrectArgumentException("incorrect type of node",
                                         __PRETTY_FUNCTION__);
    }
  }

  bool operator==(const Node& another) const {
    return type == another.type && result == another.result
      && var_name == another.var_name;
  }
};

class Tree {
 private:
  Node* root_{nullptr};
  FILE* step_by_step_{nullptr};
  size_t step_num_{0};
  mutable std::string tex_begin_;
  mutable std::string tex_body_;
  mutable std::string tex_end_;

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
        std::cout << cur_node->var_name << ' ';
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

    Node* result = allocator_.init_alloc(Node(node->type, node->result, node->var_name));
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

    if (node->type == REAL_NUMBER || (node->type == VARIABLE && node->var_name != variable)) {
      PRINT_STEP_NUM();
      PRINTLN_STEP("В это трудно поверить, но\n");
      PRINT_STEP(node->to_str());
      PRINTLN_STEP("'=0\n");

      return Tree(allocator_.init_alloc(Node(REAL_NUMBER, 0.0)));
    } else if (node->type == VARIABLE && node->var_name == variable) {
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
        operands.push_back(allocator_.init_alloc(Node(VARIABLE, 1, cur_ch)));
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

  int numerateNode(Node* node, size_t* node_index,
                   std::vector<std::pair<int, int>>* edge_list, std::vector<std::string>* labels) {
    if (node == nullptr) {
      return -1;
    }
    int left_id = numerateNode(node->left_son, node_index, edge_list, labels);
    int right_id = numerateNode(node->right_son, node_index, edge_list, labels);
    int cur_id = (*node_index)++;

    labels->push_back(node->to_str());

    if (left_id != -1) {
      edge_list->push_back({cur_id, left_id});
    }
    if (right_id != -1) {
      edge_list->push_back({cur_id, right_id});
    }
    return cur_id;
  }

  void drawNodes(const std::vector<std::string>& label_list, FILE* dot_file) {
    fprintf(dot_file, "    node [style=filled];\n");
    for (size_t node_id = 0; node_id < label_list.size(); ++node_id) {
      fprintf(dot_file, "    node%zu  [label=", node_id);
      fprintf(dot_file, "    %c%s%c];", static_cast<char>(34), label_list[node_id].c_str(), static_cast<char>(34));
    }
  }

  void drawEdges(const std::vector<std::pair<int, int>>& edge_list, FILE* dot_file) {
    for (std::pair<int, int> cur_edge: edge_list) {
      fprintf(dot_file, "    node%d  -> node%d;", cur_edge.first, cur_edge.second);
    }
  }

  void draw(FILE* dot_file) {
    std::vector<std::pair<int, int>> edge_list;
    std::vector<std::string> label_list;
    size_t node_index = 0;

    numerateNode(root_, &node_index, &edge_list, &label_list);
    fprintf(dot_file, "digraph G {\n");
    drawNodes(label_list, dot_file);
    drawEdges(edge_list, dot_file);
    fprintf(dot_file, "}");
  }
};

#endif //DED_PROG_LANG_TREE_H
