//
// Created by mike on 13.12.18.
//

#ifndef DED_PROG_LANG_STACK_ALLOCATOR_H
#define DED_PROG_LANG_STACK_ALLOCATOR_H

#include <iostream>
#include <stack>
#include <memory>

#include "exception.h"

template <class T >
class StackAllocator {
 private:
  std::stack<T*> free_memory_;

 public:

  T* allocate(size_t item_cnt = 1) {
    return new T();
  }

  T* init_alloc(const T& obj) {
    return new T(obj);
  }

  T* init_alloc(T&& obj) {
    return new T(std::move(obj));
  }

  void deallocate(T* address, size_t item_cnt) {
    T* address_last = address + item_cnt;

    for (T* cur_address = address; cur_address != address_last; ++cur_address) {
      free_memory_.push(address);
    }
  }

  ~StackAllocator() {
    while (!free_memory_.empty()) {
      T* address = free_memory_.top();
      free_memory_.pop();
      address->~T();
      delete address;
    }
  }
};

#endif //DED_PROG_LANG_STACK_ALLOCATOR_H
