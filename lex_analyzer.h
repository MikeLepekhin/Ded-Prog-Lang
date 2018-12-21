//
// Created by mike on 20.12.18.
//

#ifndef DED_PROG_LANG_LEX_ANALYZER_H
#define DED_PROG_LANG_LEX_ANALYZER_H


#include <cctype>
#include <vector>
#include <string>
#include <set>

enum TokenType {
  BRACE,
  SEPARATOR,
  KEYWORD,
  INTEGER,
  DOUBLE,
  STRING,
  OPER,
  ASSIGN
};

struct Token {
  std::string value;
  TokenType token_type;
};

class LexAnalyzer {
 private:
  const static size_t MAX_BUF_SIZE = 4096;

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
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '!';
  }

  bool isExprChar(char ch) const {
    return isBrace(ch) || isOper(ch) || ch == ';' || ch == '=';
  }

  bool isSeparator(char ch) const {
    return ch == ',' || ch == ';' || ch == '!' || ch == '&' ||
           ch == '|' || ch == '<' || ch == '>' || ch == '=';
  }

  std::set<std::string> keywords_;

 public:
  LexAnalyzer(FILE* input) {
    buf_size_ = fread((void*)buf_, sizeof(char), MAX_BUF_SIZE - 1, input);
    buf_ptr_ = buf_;
    std::cout << "size of code buffer: " << buf_size_ << '\n';

    keywords_.insert("lol");
    keywords_.insert("kek");
    keywords_.insert("func");
    keywords_.insert("int");
    keywords_.insert("float");
    keywords_.insert("var");
    keywords_.insert("if");
    keywords_.insert("while");
    keywords_.insert("else");
    keywords_.insert("main");
    keywords_.insert("sin");
    keywords_.insert("cos");
  }

  bool done() const {
    return buf_ptr_ == buf_ + buf_size_;
  }

  Token parseInt() {
    std::string result = "";

    while (!done() && std::isdigit(*buf_ptr_)) {
      result.push_back(*buf_ptr_);
      ++buf_ptr_;
    }
    return {result, INTEGER};
  }

  Token parseDouble() {
    std::string result = "";
    Token int_part = parseInt();

    if (done() || *buf_ptr_ != '.' || int_part.value == "") {
      return int_part;
    }
    result = int_part.value + ".";
    ++buf_ptr_;
    while (!done() && std::isdigit(*buf_ptr_)) {
      result.push_back(*buf_ptr_);
      ++buf_ptr_;
    }
    return {result, DOUBLE};
  }

  Token parseBrace() {
    if (isBrace(*buf_ptr_)) {
      Token result{std::string("") + *buf_ptr_, BRACE};

      ++buf_ptr_;
      return result;
    }
    return {"", BRACE};
  }

  Token parseString(const std::string& str, TokenType token_type = STRING) {
    size_t char_id = 0;
    char* was_ptr = buf_ptr_;

    while (!done() && char_id < str.size()) {
      if (str[char_id] != *buf_ptr_) {
        break;
      }
      ++char_id;
      ++buf_ptr_;
    }
    if (char_id < str.size()) {
      buf_ptr_ = was_ptr;
      return {"", token_type};
    }

    return {str, token_type};
  }


  Token parseOper() {
    Token result;

    std::vector<std::string> oper_strs{"==", "!=", "!", "<=", ">=", "<", ">",
                                       "||", "&&", "+=", "-=", "*=", "/="};

    for (const std::string& oper_str: oper_strs) {
      result = result = parseString(oper_str, OPER);
      if (result.value != "") {
        break;
      }
    }
    return result;
  }

  Token parseSeparator() {
    if (*buf_ptr_ == ';') {
      Token result{std::string("") + *buf_ptr_, SEPARATOR};

      ++buf_ptr_;
      return result;
    }
    if (*buf_ptr_ == ',') {
      Token result{std::string("") + *buf_ptr_, SEPARATOR};

      ++buf_ptr_;
      return result;
    }
    return {"", SEPARATOR};
  }

  Token parseAssign() {
    if (*buf_ptr_ == '=') {
      Token result{std::string("") + *buf_ptr_, ASSIGN};

      ++buf_ptr_;
      return result;
    }
    return {"", ASSIGN};
  }

  Token parseName() {
    std::string result = "";

    while (!done() && !std::isspace(*buf_ptr_) && !isBrace(*buf_ptr_) && !isSeparator(*buf_ptr_)) {
      result.push_back(*buf_ptr_);
      ++buf_ptr_;
    }

    return {result, STRING};
  }

  Token parseKeyword() {
    Token result = parseName();
    if (keywords_.find(result.value) != keywords_.end()) {
      result.token_type = KEYWORD;
    }
    return result;
  }

  void parseToken(std::vector<Token>& tokens) {
    Token result = parseBrace();

    if (result.value == "") {
      result = parseDouble();
    }
    if (result.value == "") {
      result = parseOper();
    }
    if (result.value == "") {
      result = parseSeparator();
    }
    if (result.value == "") {
      result = parseKeyword();
    }
    if (result.value == "") {
      result = parseName();
    }
    if (result.value == "") {
      result = parseAssign();
    }

    if (result.value == "") {
      throw IncorrectParsingException(std::string("say whaaat? ") + *buf_ptr_,
                                      __PRETTY_FUNCTION__);
    } else {
      std::cout << tokens.size() << ": " << result.value << ' ' << result.token_type << '\n';
      tokens.push_back(result);
    }
  }

  void parseTokens(std::vector<Token>& tokens) {
    try {
      skipSpaceChars();
      while (!done()) {
        parseToken(tokens);
        skipSpaceChars();
      }
    } catch (InterpreterException& exc) {
      throw exc;
    }
  }
};

#endif //DED_PROG_LANG_LEX_ANALYZER_H
