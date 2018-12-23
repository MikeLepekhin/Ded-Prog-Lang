//
// Created by mike on 22.12.18.
//

#ifndef DED_PROG_LANG_VISUALIZER_H
#define DED_PROG_LANG_VISUALIZER_H

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "tree.h"

struct VisualizerException : public std::exception {
  std::string message;
  std::string function_name;

  VisualizerException(const std::string& message = "", const std::string& function_name = ""):
    message(message), function_name(function_name) {}
};

std::ostream& operator<<(std::ostream& os, const VisualizerException& exception) {
  os << "!!! exception: " << ' ' << exception.message;
  if (!exception.function_name.empty()) {
    os << "(" << exception.function_name << ")";
  }
  os << '\n';
  return os;
}

struct VisFuncBlock {
  std::vector<std::string> params;
  std::vector<std::string> variables;
};

class Visualizer {
 private:
  const static size_t MAX_BUF_SIZE = 4096;

  char buf_[MAX_BUF_SIZE];
  size_t buf_size_;
  char* buf_ptr_;
  Tree tree_;
  FILE* tree_file_;

  void skipSpaceChars() {
    while (!done() && std::isspace(*buf_ptr_)) {
      ++buf_ptr_;
    }
  }

  std::vector<std::string> variables_;
  std::vector<std::string> funcs_;
  std::vector<VisFuncBlock> func_blocks_;
  size_t node_cnt_ = 0;

 public:
  Visualizer(FILE* input) {
    buf_size_ = fread((void*)buf_, sizeof(char), MAX_BUF_SIZE - 1, input);
    buf_ptr_ = buf_;
    std::cout << "size of code buffer: " << buf_size_ << '\n';
  }

  bool done() const {
    return buf_ptr_ == buf_ + buf_size_;
  }

  std::string parseInt() {
    std::string result = "";

    skipSpaceChars();
    while (!done() && std::isdigit(*buf_ptr_)) {
      result.push_back(*buf_ptr_);
      ++buf_ptr_;
    }
    skipSpaceChars();
    return result;
  }

  std::string parseDouble() {
    std::string result = parseInt();

    if (done() || *buf_ptr_ != '.' || result == "") {
      return result;
    }
    result.push_back('.');
    ++buf_ptr_;
    while (!done() && std::isdigit(*buf_ptr_)) {
      result.push_back(*buf_ptr_);
      ++buf_ptr_;
    }
    skipSpaceChars();
    return result;
  }

  bool parseString(const std::string& str) {
    size_t char_id = 0;
    char* was_ptr = buf_ptr_;

    skipSpaceChars();
    while (!done() && char_id < str.size()) {
      if (str[char_id] != *buf_ptr_) {
        break;
      }
      ++char_id;
      ++buf_ptr_;
    }
    if (char_id < str.size()) {
      buf_ptr_ = was_ptr;
      return false;
    }
    skipSpaceChars();

    return true;
  }

  std::string parseName() {
    std::string result = "";

    skipSpaceChars();
    while (!done() && !std::isspace(*buf_ptr_) && *buf_ptr_ != ']') {
      result.push_back(*buf_ptr_);
      ++buf_ptr_;
    }
    skipSpaceChars();

    return result;
  }

  Node* parseNode() {
    try {
      std::cout << "begin\n";
      if (!parseString("[")) {
        throw VisualizerException(std::string("[ was expected but not ") + *buf_ptr_,
                                  __PRETTY_FUNCTION__);
      }

      NodeType node_type = static_cast<NodeType>(atoi(parseInt().c_str()));
      double node_value = atof(parseDouble().c_str());
      Node* result_node = Tree::allocator_.init_alloc(Node{node_type, node_value});

      std::cout << node_type << ' ' << node_value << '\n';

      while (!done() && *buf_ptr_ == '[') {
        Node* son_node = parseNode();
        result_node->sons.push_back(son_node);
      }
      //std::cout << buf_ + buf_size_ - buf_ptr_ << '\n';

      if (!parseString("]")) {
        throw VisualizerException(std::string("] was expected but not ") + *buf_ptr_,
                                  __PRETTY_FUNCTION__);
      }
      std::cout << "end\n";
      return result_node;
    } catch (VisualizerException& exc) {
      throw exc;
    }
  }

  void makeTree() {
    try {
      if (!parseString("VARS")) {
        throw VisualizerException("where is VARS?", __PRETTY_FUNCTION__);
      }
      size_t var_cnt = atoi(parseInt().c_str());

      for (size_t var_id = 0; var_id < var_cnt; ++var_id) {
        variables_.push_back(parseName());
      }

      if (!parseString("FUNCS")) {
        throw VisualizerException("where is FUNCS?", __PRETTY_FUNCTION__);
      }
      size_t func_cnt = atoi(parseInt().c_str());
     // std::cerr << "func_cnt " << func_cnt << '\n';

      for (size_t func_id = 0; func_id < func_cnt; ++func_id) {
        std::string func_name = parseName();
        if (func_name.empty()) {
          throw VisualizerException("function name is empty", __PRETTY_FUNCTION__);
        }
        std::cerr << "function " << func_name << '\n';

        func_name.erase(func_name.size() - 1, 1);
        funcs_.push_back(func_name);
        func_blocks_.push_back(VisFuncBlock());

        if (!parseString("PARAMS")) {
          std::cerr << *buf_ptr_ << '\n';
          throw VisualizerException("where is PARAMS?", __PRETTY_FUNCTION__);
        }
        size_t param_cnt = atoi(parseInt().c_str());
        std::cerr << "param cnt " << param_cnt << '\n';
        for (size_t param_id = 0; param_id < param_cnt; ++param_id) {
          std::string param_name = parseName();

          func_blocks_[func_id].params.push_back(param_name);
          std::cerr << "param " << param_name << '\n';
        }

        if (!parseString("NEWVAR")) {
          std::cerr << *buf_ptr_ << '\n';
          throw VisualizerException("where is NEWVAR?", __PRETTY_FUNCTION__);
        }
        size_t var_cnt = atoi(parseInt().c_str());
        std::cerr << "var cnt " << var_cnt << '\n';
        for (size_t var_id = 0; var_id < var_cnt; ++var_id) {
          std::string param_name = parseName();

          func_blocks_[func_id].variables.push_back(param_name);
          std::cerr << "var " << param_name << '\n';
        }

      }
      tree_.setRoot(parseNode());
      std::cout << "root was parsed\n";
    } catch (VisualizerException& exc) {
      std::cerr << exc;
    }
  }

  size_t showRec(Node* node, FILE* file, FILE* code_file, int func_id) {
    size_t cur_num = node_cnt_++;
    fprintf(file, "node%zu [label=%c", cur_num, static_cast<char>(34));
    double value = node->value;
    int int_value = static_cast<int>(node->value);

    std::cerr << node->type << ' ' << node->value << '\n';
    std::cerr << func_id << '\n';

    switch (node->type) {
      case ROOT:
        fprintf(file, "root");
        break;
      case FUNCS:
        fprintf(file, "funcs");
        break;
      case USER_FUNCTION:
        fprintf(file, "%s", funcs_[int_value].c_str());
        fprintf(code_file, "%s(", funcs_[int_value].c_str());
        if (func_id == -1) {
          func_id = int_value;

        }
        fprintf(code_file, ")");
        break;
      case NUMBER:
        if (value != int_value) {
          fprintf(file, "%.6f", value);
          fprintf(code_file, "%.6f", value);
        } else {
          fprintf(file, "%d", int_value);
          fprintf(code_file, "%.6f", value);
        }
        break;
      case VARIABLE:
        if (func_id == -1) {
          fprintf(file, "%s", variables_[int_value].c_str());
          fprintf(code_file, "%s", variables_[int_value].c_str());
        } else {
          fprintf(file, "%s", func_blocks_[func_id].variables[int_value].c_str());
          fprintf(code_file, "%s", func_blocks_[func_id].variables[int_value].c_str());
        }
        break;
      case LOCAL_VARIABLE:
        fprintf(file, "%s", func_blocks_[func_id].variables[int_value].c_str());
        fprintf(code_file, "%s", func_blocks_[func_id].variables[int_value].c_str());
        break;
      case OPERATOR:
      {
        int operator_type = static_cast<int>(node->value);

        switch (operator_type) {
          case EQUAL:
            fprintf(file, "=");
            fprintf(code_file, " = ");
            break;
          case PLUS:
            fprintf(file, "+");
            fprintf(code_file, " + ");
            break;
          case MINUS:
            fprintf(file, "-");
            fprintf(code_file, " - ");
            break;
          case MULTIPLY:
            fprintf(file, "*");
            fprintf(file, " * ");
            break;
          case DIVIDE:
            fprintf(file, "/");
            fprintf(file, " / ");
            break;
          case POWER:
            fprintf(file, "^");
            fprintf(file, " ^ ");
            break;
          case BOOL_EQUAL:
            fprintf(file, "==");
            fprintf(file, " == ");
            break;
          case BOOL_NOT_EQUAL:
            fprintf(file, "!=");
            fprintf(file, " != ");
            break;
          case BOOL_LOWER:
            fprintf(file, "<");
            fprintf(file, " < ");
            break;
          case BOOL_GREATER:
            fprintf(file, ">");
            fprintf(file, " > ");
            break;
          case BOOL_NOT_LOWER:
            fprintf(file, ">=");
            fprintf(file, " >= ");
            break;
          case BOOL_NOT_GREATER:
            fprintf(file, "<=");
            fprintf(file, " <= ");
            break;
          case BOOL_NOT:
            fprintf(file, "!");
            fprintf(file, "!");
            break;
          case BOOL_OR:
            fprintf(file, "||");
            break;
          case BOOL_AND:
            fprintf(file, "&&");
            break;
          case PLUS_EQUAL:
            fprintf(file, "+=");
            break;
          case MINUS_EQUAL:
            fprintf(file, "-=");
            break;
          case MULTIPLY_EQUAL:
            fprintf(file, "*=");
            break;
          case DIVIDE_EQUAL:
            fprintf(file, "/=");
            break;
        }
        break;
      }
      case LOGIC:
      {
        int logic_type = static_cast<int>(node->value);

        switch (logic_type) {
          case IF:
            fprintf(file, "if");
            break;
          case ELSE:
            fprintf(file, "else");
            break;
          case WHILE:
            fprintf(file, "while");
            break;
          case CONDITION:
            fprintf(file, "condition");
            break;
          case CONDITION_MET:
            fprintf(file, "condition_met");
            break;
        }
        break;
      }
      case MAIN:
        func_id = funcs_.size() - 1;
        fprintf(file, "main");
        break;
      case STANDART_FUNCTION:
      {
        int func_type = static_cast<int>(node->value);

        switch (func_type) {
          case INPUT:
            fprintf(file, "input");
            break;
          case OUTPUT:
            fprintf(file, "output");
            break;
          case SIN:
            fprintf(file, "sin");
            break;
          case COS:fprintf(file, "cos");
            break;
          case CALL: {
            fprintf(file, "call");
            break;
          }
          case SQ_ROOT:fprintf(file, "sqrt");
            break;
        }
        break;
      }
      case VAR_INIT:
        fprintf(file, "var_init");
        break;
      case RETURN:
        fprintf(file, "return");
        fprintf(code_file, "return");
        break;
      case PARAM:
        fprintf(file, "%s", func_blocks_[func_id].params[int_value].c_str());
        break;

      default:
        break;
    }
    fprintf(file, "%c];", static_cast<char>(34));

    if (node->type == STANDART_FUNCTION && node->value == CALL) {
      int cur_func_id = node->sons[0]->value;

      for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
        size_t son_num = showRec(node->sons[son_id], file, code_file, cur_func_id);

        fprintf(file, "  node%zu->node%zu;\n", cur_num, son_num);
      }
      return cur_num;
    }
    for (size_t son_id = 0; son_id < node->sons.size(); ++son_id) {
      size_t son_num = showRec(node->sons[son_id], file, code_file, func_id);

      fprintf(file, "  node%zu->node%zu;\n", cur_num, son_num);
    }
    return cur_num;
  }

  void show(const std::string& tree_filename, const std::string& code_filename) {
    SmartFile tree_file(tree_filename.c_str(), "w");
    SmartFile smart_code_file(code_filename.c_str(), "w");
    FILE* file = tree_file.getFile();
    FILE* code_file = smart_code_file.getFile();

    fprintf(file, "digraph G {\n");
    fprintf(file, "  node [style=filled];\n");
    showRec(tree_.getRoot(), file, code_file, -1);
    fprintf(file, "}");
    tree_file.release();

    std::string command_text = "xdot " + tree_filename;
    std::cout << command_text << '\n';

    system(command_text.c_str());
  }
};

#endif //DED_PROG_LANG_VISUALIZER_H
