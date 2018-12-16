//
// Created by mike on 16.12.18.
//

#ifndef DED_PROG_LANG_NORMALIZER_H
#define DED_PROG_LANG_NORMALIZER_H

#include <cctype>

class Normalizer {
 private:
  const static size_t MAX_BUF_SIZE = 4096;

  FILE* output_;
  char buf_[MAX_BUF_SIZE];
  size_t buf_size_;
  char* buf_ptr_;

  void skipSpaceChars() {
    while (std::isspace(*buf_ptr_)) {
      ++buf_ptr_;
    }
  }

  void skipNewLineChars() {
    while (*buf_ptr_ == '\n') {
      ++buf_ptr_;
    }
  }

  bool isBrace(char ch) const {
    return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}';
  }

  bool isOper(char ch) const {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/';
  }

  bool isExprChar(char ch) const {
    return isBrace(ch) || isOper(ch) || ch == ';' || ch == '=';
  }

 public:
  Normalizer(FILE* input, FILE* output):
      output_(output) {
    buf_size_ = fread((void*)buf_, sizeof(char), MAX_BUF_SIZE - 1, input);
    buf_ptr_ = buf_;
  }

  bool done() const {
    return buf_ptr_ == buf_ + buf_size_;
  }

  void parseLine() {
    skipSpaceChars();
    char last_char = 0;

    while (*buf_ptr_ != '\n' && *buf_ptr_ != '\0') {
      if (std::isspace(*buf_ptr_) && !isExprChar(last_char)) {
        last_char = ' ';
      } else if (!std::isspace(*buf_ptr_)) {
        if (last_char != 0 && last_char == ' ' && !isExprChar(*buf_ptr_)) {
          fprintf(output_, "%c", last_char);
        }
        fprintf(output_, "%c", *buf_ptr_);
        last_char = *buf_ptr_;
      }
      ++buf_ptr_;
    }

  }

  void normalize() {
    while (!done()) {
      parseLine();
      fprintf(output_, "\n");
      skipNewLineChars();
    }
  }
};

#endif //DED_PROG_LANG_NORMALIZER_H
