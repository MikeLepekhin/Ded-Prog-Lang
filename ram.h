//
// Created by mike on 16.12.18.
//

#ifndef DED_PROG_LANG_RAM_H
#define DED_PROG_LANG_RAM_H

#include <cstdio>
#include <iostream>
#include <array>
#include <thread>
#include <chrono>

#include "exception.h"

template<class T, size_t SIZE = 1000000>
class RAM {
 private:
  std::array<T, SIZE> memory_cells;

 public:

  T getValue(size_t address) {
    if (address > SIZE) {
      throw OutOfRangeException("incorrect memory address", __PRETTY_FUNCTION__);
    }

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    return memory_cells[address];
  }

  void setValue(size_t address, const T& value) {
    if (address > SIZE) {
      throw OutOfRangeException("incorrect memory address", __PRETTY_FUNCTION__);
    }

    //std::this_thread::sleep_for(std::chrono::seconds(2));
    memory_cells[address] = value;
  }
};


#endif //DED_PROG_LANG_RAM_H
