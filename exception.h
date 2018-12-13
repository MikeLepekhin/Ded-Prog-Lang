//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_EXCEPTION_H
#define DED_PROG_LANG_EXCEPTION_H

#include <iostream>
#include <exception>
#include <string>

struct InterpreterException : public std::exception {
  std::string message;
  std::string function_name;

  InterpreterException(const std::string& message = "", const std::string& function_name = ""):
    message(message), function_name(function_name) {}

  virtual std::string getLabel() const {
    return "Exception";
  }
};

struct IncorrectArgumentException : public InterpreterException {
  IncorrectArgumentException(const std::string& message, const std::string& function_name = ""):
    InterpreterException(message, function_name) {}

  std::string getLabel() const {
    return "IncorrectArgumentException";
  }
};

struct IncorrectParsingException : public InterpreterException {
  IncorrectParsingException(const std::string& message, const std::string& function_name = ""):
    InterpreterException(message, function_name) {}

  std::string getLabel() const {
    return "IncorrectParsingException";
  }
};

struct DivisionByZeroException : public InterpreterException {
  DivisionByZeroException(const std::string& message, const std::string& function_name = ""):
    InterpreterException(message, function_name) {}

  std::string getLabel() const {
    return "DivisionByZeroException";
  }
};

std::ostream& operator<<(std::ostream& os, const InterpreterException& iaexception) {
  os << "!!! " << iaexception.getLabel() << ' ' << iaexception.message;
  if (!iaexception.function_name.empty()) {
    os << "(" << iaexception.function_name << ")";
  }
  os << '\n';
  return os;
}

#endif //DED_PROG_LANG_EXCEPTION_H
