//
// Created by mike on 16.12.18.
//

#ifndef DED_PROG_LANG_EXECUTOR_H
#define DED_PROG_LANG_EXECUTOR_H

#define NDEBUG

#include <iostream>
#include <vector>
#include <cmath>

#include "stack.h"
#include "ram.h"
#include "file_buffer.h"
#include "common_classes.h"

const size_t REGISTER_COUNT = 16;
const size_t COMMAND_COUNT = 30;
const size_t MAX_ARG_COUNT = 2;

template<class T = double>
class Processor {
 private:
  T registers_[REGISTER_COUNT];
  Stack<T> stack_;
  Stack<size_t> instruction_stack_;
  RAM<T> ram_;
  FileBuffer fbuffer_;

  size_t instruction_pointer_{0};

  std::vector<Command<T>> commands;



  void parseCommand(size_t cmd_id, size_t arg_cnt, Command<T>& command) {
    command.cmd_id = cmd_id;
    command.arg_cnt = arg_cnt;
    //std::cout << "parsing of command" << cmd_id << " count of arguments: " << arg_cnt << '\n';

    for (size_t arg_id = 0; arg_id < arg_cnt; ++arg_id) {
      int cur_type = fbuffer_.readFromBuffer<int>();
      T cur_val = fbuffer_.readFromBuffer<T>();

      command.args.push_back({cur_val, cur_type});
      // std::cout << "argument " << cur_type << ' ' << cur_val << '\n';
    }
  }

  void parseAll() {

    while (!fbuffer_.done()) {
      size_t cmd_id = fbuffer_.readFromBuffer<size_t>();
      size_t arg_cnt = fbuffer_.readFromBuffer<size_t>();

      if (arg_cnt > MAX_ARG_COUNT || cmd_id > COMMAND_COUNT) {
        throw IncorrectArgumentException(std::string("incorrect cmd code or argument cnt ")
                                           + std::to_string(cmd_id) + ' ' + std::to_string(arg_cnt),
                                         __PRETTY_FUNCTION__);
      }

      commands.push_back(Command<T>());
      parseCommand(cmd_id, arg_cnt, commands.back());
    }
  }

  T getRamAddress(int arg_value) {
    int reg_num = arg_value >> 8;
    int shift = arg_value & ((1 << 8) - 1);

    return static_cast<int>(registers_[reg_num]) + shift;
  }

  T getArgumentValue(std::pair<T, int>& arg) {
    switch (arg.second) {
      case 1:
        return arg.first;
      case 2:
        return registers_[getIntValue(arg)];
      case 3:
        return ram_.getValue(getRamAddress(arg.first));
      default:
        throw IncorrectArgumentException("", __PRETTY_FUNCTION__);
    }
  }

  int getIntValue(std::pair<T, int>& arg) {
    return static_cast<int>(arg.first);
  }

  void inCmd(T& value) {
    std::cout << "# enter a value, please\n";
    std::cin >> value;
  }

  void outCmd(const T& value) const {
    std::cout << "# console out: " << value << "\n";
  }

 public:
  Processor(FILE* binary_file): fbuffer_(binary_file) {
    parseAll();
  }

  void executeCommand() {
    Command<T>& cur_command = commands[instruction_pointer_];

    switch (cur_command.cmd_id) {
#define COMMAND(cmd_id, name, arg_cnt, arg_mask, source_cmd) \
    case cmd_id:\
    {\
      /*std::cout << "execute command " << cmd_id << "\n";*/\
      source_cmd\
      break;\
    }

#include "commands.h"
#undef COMMAND
      default:
        throw IncorrectArgumentException(std::string("unknown command code") +
                                           std::to_string(cur_command.cmd_id),
                                         __PRETTY_FUNCTION__);
    }
    ++instruction_pointer_;
  }

  bool isDone() const {
    return instruction_pointer_ == commands.size();
  }

  void executeAll() {
    while (!isDone()) {
      executeCommand();
    }
    std::cout << "# processor: execution is finished\n";
  }
};

void execute(FILE* binary_file) {
  Processor<> processor(binary_file);

  processor.executeAll();
}

#endif //DED_PROG_LANG_EXECUTOR_H
