//
// Created by mike on 16.12.18.
//

#ifndef DED_PROG_LANG_ASSEMBLER_H
#define DED_PROG_LANG_ASSEMBLER_H

#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <unordered_map>
#include <ctype.h>

#include "common_classes.h"
#include "exception.h"

const size_t ARG_SIZE = 30;

bool isDigit(char ch) {
  return ch >= '0' && ch <= '9';
}

bool isIntNumber(char arg[ARG_SIZE]) {
  size_t len = strlen(arg);
  bool is_number = true;

  for (size_t char_id = 0; char_id < len; ++char_id) {
    is_number &= (isDigit(arg[char_id]));
  }
  return is_number;
}

bool isFloatNumber(char arg[ARG_SIZE]) {
  size_t len = strlen(arg);
  bool is_number = true;
  size_t dot_cnt = 0;

  for (size_t char_id = 0; char_id < len; ++char_id) {
    is_number &= (isDigit(arg[char_id]) || arg[char_id] == '.');
    dot_cnt += (arg[char_id] == '.');
  }
  return is_number && dot_cnt <= 1;
}

int regNum(char arg[ARG_SIZE]) {
  size_t len = strlen(arg);

  if (len < 2|| len > 3 || arg[0] != 'r') {
    return -1;
  }
  if (len == 2) {
    return (arg[1] >= '0' && arg[1] <= '9' ? arg[1] - '0' : -1);
  } else if (arg[2] == 'x') {
    return (arg[1] >= 'a' && arg[1] <= 'e' ? arg[1] - 'a' + 1 : -1);
  } else if (arg[1] == '1') {
    return (arg[2] >= '0' && arg[2] <= '5' ? 10 + (arg[2] - '0') : -1);
  } else {
    return -1;
  }
}

std::pair<int, int> objectRAM(char arg[ARG_SIZE]) {
  size_t len = strlen(arg);

  if (len < 2 || arg[0] != '[' || arg[len - 1] != ']') {
    return {-1, -1};
  }
  size_t plus_cnt = 0;
  int plus_pos = -1;
  for (size_t char_id = 1; char_id < len - 1; ++char_id) {
    if (arg[char_id] == '+') {
      ++plus_cnt;
      plus_pos = char_id;
    }
  }
  if (plus_cnt == 0) {
    char value[ARG_SIZE];

    for (size_t char_id = 1; char_id < len - 1; ++char_id) {
      value[char_id - 1] = arg[char_id];
    }
    value[len - 2] = 0;

    if (isIntNumber(value)) {
      return {0, atoi(value)};
    } else {
      return {regNum(value), 0};
    }
  } else if (plus_cnt == 1) {
    char value1[ARG_SIZE];
    char value2[ARG_SIZE];

    for (size_t char_id = 1; char_id < plus_pos; ++char_id) {
      value1[char_id - 1] = arg[char_id];
    }
    value1[plus_pos - 1] = 0;

    for (size_t char_id = plus_pos + 1; char_id < len - 1; ++char_id) {
      value2[char_id - (plus_pos + 1)] = arg[char_id];
    }
    value2[len - 1 - (plus_pos + 1)] = 0;

    return {regNum(value1), atoi(value2)};
  } else {
    return {-1, -1};
  }
}

void parseCommand(size_t cmd_id, const std::string& cmd, size_t arg_cnt, size_t support_mask,
                  std::vector<std::pair<double, int>>& arg_values, std::vector<std::string>& label_request,
                  FILE* asm_file) {

  char arg[ARG_SIZE];

  if (isJump(cmd)) {
    fscanf(asm_file, "%s", arg);
    label_request.push_back(std::string(arg));
    return;
  }

  for (size_t arg_id = 0; arg_id < arg_cnt; ++arg_id) {
    fscanf(asm_file, "%s", arg);

    if (isFloatNumber(arg)) {
      if (!(support_mask & 1)) {
        throw IncorrectArgumentException("numbers are not allowed as arguments of " + cmd);
      }
      arg_values.push_back({atof(arg), 1});
    } else if (regNum(arg) != -1) {
      if (!(support_mask & 2)) {
        throw IncorrectArgumentException("registers are not allowed as arguments of " + cmd);
      }
      arg_values.push_back({regNum(arg), 2});
    } else {
      if (!(support_mask & 4)) {
        throw IncorrectArgumentException("RAM is not allowed as argument of " + cmd);
      }
      std::pair<int, int> ram_obj = objectRAM(arg);

      if (ram_obj.first == -1 || ram_obj.second == -1) {
        throw IncorrectArgumentException("incorrect argument: " + std::string(arg) + " for command " + cmd,
                                         __PRETTY_FUNCTION__);
      }
      arg_values.push_back({(ram_obj.first << 8) + ram_obj.second, 3});
    }
  }
}

void encodeCommand(const Command<double>& cmd, FILE* binary_file) {
  fwrite(&cmd.cmd_id, sizeof(size_t), 1, binary_file);
  fwrite(&cmd.arg_cnt, sizeof(size_t), 1, binary_file);
  //std::cout << "cmd " << cmd.cmd_id << ' ' << cmd.arg_cnt << '\n';

  for (size_t arg_id = 0; arg_id < cmd.arg_cnt; ++arg_id) {
    fwrite(&cmd.args[arg_id].second, sizeof(int), 1, binary_file);
    fwrite(&cmd.args[arg_id].first, sizeof(double), 1, binary_file);
   // std::cout << cmd.args[arg_id].second << ' ' << cmd.args[arg_id].first << '\n';
  }
}

void assemblyCommand(size_t cmd_id, const std::string& cmd, size_t arg_cnt, size_t support_mask,
                     std::vector<Command<double>>& commands, std::vector<std::string>& label_request,
                     FILE* asm_file) {

  std::vector<std::pair<double, int>> args;

  parseCommand(cmd_id, cmd, arg_cnt, support_mask, args, label_request, asm_file);
  commands.push_back(Command<double>{cmd_id, cmd, arg_cnt, args});
  //encodeCommand(cmd_id, arg_cnt, arg_values, binary_file);
}

void removeSpaceChars(std::string& str) {
  std::string result = "";
  for (char c: str) {
    if (!isspace(c)) {
      result.push_back(c);
    }
  }
  str = result;
}

void assembly(FILE* asm_file = stdin, FILE* binary_file = stdout) {
  std::vector<Command<double>> commands;

  std::unordered_map<std::string, int> jump_to;
  std::vector<std::string> label_request;
  size_t cur_label_request = 0;

  while (!feof(asm_file)) {
    char cmd_buf[ARG_SIZE];
    if (fscanf(asm_file, "%s", cmd_buf) <= 0) {
      break;
    }

    std::string cmd = cmd_buf;
    //std::cout << cmd << '\n';

    removeSpaceChars(cmd);
    if (cmd == "") {
      continue;
    }
    if (cmd[0] == ':') {
      cmd = cmd.erase(0, 1);
      if (jump_to.find(cmd) != jump_to.end()) {
        throw IncorrectArgumentException("double declaration of label " + cmd, __PRETTY_FUNCTION__);
      }
      jump_to[cmd] = commands.size();
      continue;
    }

    if (cmd == "move") {
      assemblyCommand(1, "push", 1, 7, commands, label_request, asm_file);
      assemblyCommand(2, "pop",  1, 6, commands, label_request, asm_file);
      continue;
    }

    if (false) {

    }
#define COMMAND(cmd_id, name, arg_cnt, arg_mask, cmd_source) \
    else if (cmd == name) {\
      assemblyCommand(cmd_id, name, arg_cnt, arg_mask, commands, label_request, asm_file);\
    }
#include "commands.h"
#undef COMMAND
    else {
      throw IncorrectArgumentException(std::string("incorrect command ") + cmd);
    }
  }
  assemblyCommand(11, "end", 0, 0, commands, label_request, asm_file);


  for (const std::string& label: label_request) {
    if (jump_to.find(label) == jump_to.end()) {
      throw IncorrectArgumentException("incorrect label value" + label, __PRETTY_FUNCTION__);
    }
  }

  for (Command<double>& command: commands) {
    if (isJump(command.cmd_name)) {
      std::string cur_label = label_request[cur_label_request++];
      command.args.push_back({jump_to[cur_label], 1});
    }
    encodeCommand(command, binary_file);
  }
}


#endif //DED_PROG_LANG_ASSEMBLER_H
