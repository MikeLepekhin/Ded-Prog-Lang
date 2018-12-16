//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_EXCEPTION_H
#define DED_PROG_LANG_EXCEPTION_H

#include <iostream>
#include <exception>
#include <string>

//#define DEBUG_LOG

#ifdef DEBUG_LOG
#define LOG(str) std::cout << str << '\n'
#endif
#ifndef DEBUG_LOG
#define LOG(str) {}
#endif

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

struct UndefinedVariableException : public InterpreterException {
  UndefinedVariableException(const std::string& message, const std::string& function_name = ""):
    InterpreterException(message, function_name) {}

  std::string getLabel() const {
    return "IncorrectArgumentException";
  }
};

struct RedefinedVariableException : public InterpreterException {
  RedefinedVariableException(const std::string& message, const std::string& function_name = ""):
    InterpreterException(message, function_name) {}

  std::string getLabel() const {
    return "IncorrectArgumentException";
  }
};

struct ProcessorException : public InterpreterException {
  std::string message;
  std::string function_name;

  ProcessorException(const std::string& message = "", const std::string& function_name = ""):
    InterpreterException(message, function_name) {}
};


std::ostream& operator<<(std::ostream& os, const ProcessorException& iaexception) {
  os << "!!! Exception: " << iaexception.message;
  if (!iaexception.function_name.empty()) {
    os << "(" << iaexception.function_name << ")";
  }
  os << '\n';
  return os;
}

struct StackException : public ProcessorException {
  StackException(const std::string& message = "", const std::string& function_name = ""):
    ProcessorException(message, function_name) {}
};

struct IncorrectStackArgumentException : public StackException {
  IncorrectStackArgumentException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
};

struct OutOfRangeException : public StackException {
  OutOfRangeException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
};

struct EmptyStackException : public StackException {
  EmptyStackException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
};

struct CanaryException : public StackException {
  CanaryException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
};

struct HashSumException : public StackException {
  HashSumException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
};

struct ParamsPoisonedException : public StackException {
  ParamsPoisonedException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
};

struct IncorrectPointerException : public StackException {
  IncorrectPointerException(const std::string& message, const std::string& function_name = ""):
    StackException(message, function_name) {}
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
