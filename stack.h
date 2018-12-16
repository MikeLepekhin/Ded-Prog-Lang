//
// Created by mike on 16.12.18.
//

#ifndef DED_PROG_LANG_HEADER_H
#define DED_PROG_LANG_HEADER_H

#include <cstdint>
#include <utility>

#include "exception.h"

#ifndef NDEBUG
#define CALC_HASHES() { hash_sum_ = calcHashSum(); full_hash_ = calcFullHash(); }
#define ASSERT_CORRECTNESS() { assertCorrectness(__PRETTY_FUNCTION__); }
#define ASSERT_PARAMS() { assertParams(__PRETTY_FUNCTION__); }
#define ASSERT_POINTERS() { assertPointers(__PRETTY_FUNCTION__); }
#else
#define CALC_HASHES() {}
  #define ASSERT_CORRECTNESS() {}
#endif

template<class T>
class Stack {
 private:
  static const size_t EXPANSION_COEFF = 2;
  static const size_t SHRINKAGE_COEFF = 2;
  constexpr static double MAX_LOAD_FACTOR = 0.5;
  constexpr static double MIN_LOAD_FACTOR = 0.25;
  static const size_t MIN_CAPACITY = 8;
#ifndef NDEBUG
  static const size_t CANARY_SIZE = 4;
  static const uint32_t CANARY_INIT_VALUE = 1983776228;

  size_t main_canary_{CANARY_INIT_VALUE};

#else
  static const size_t CANARY_SIZE = 0;
#endif

  char* bytes_{nullptr};
  size_t capacity_{0};
  size_t item_count_{0};
  char* items_begin_{nullptr};
#ifndef NDEBUG
  size_t hash_sum_{0};
  char* bytes_copy_{nullptr};
  std::hash<T> hasher_;
  size_t full_hash_{0};

  uint32_t* getCanaryPtr1() const {
    return reinterpret_cast<uint32_t*>(bytes_);
  }

  uint32_t* getCanaryPtr2() const {
    return reinterpret_cast<uint32_t*>(items_begin_ + capacity_ * sizeof(T));
  }

  void initCanaries() {
    main_canary_ = CANARY_INIT_VALUE;
#ifndef DEBUG
    ASSERT_POINTERS();
#endif
    *getCanaryPtr1() = CANARY_INIT_VALUE;
    *getCanaryPtr2() = CANARY_INIT_VALUE;
  }
#endif
  const T* getElementPtr(size_t pos) const {
    return reinterpret_cast<T*>(items_begin_ + pos * sizeof(T));
  }

  T* getElementPtr(size_t pos) {
    return reinterpret_cast<T*>(items_begin_ + pos * sizeof(T));
  }

  void copyItems(const char* from, char* to) {
    if (from == nullptr || to == nullptr) {
      throw IncorrectPointerException("pointer equals nullptr", __PRETTY_FUNCTION__);
    }

    ASSERT_CORRECTNESS();
#ifndef NDEBUG
    std::cerr << "copy items begin\n";
#endif

    const T* cur_item = reinterpret_cast<const T*>(from + CANARY_SIZE);
    T* new_item = reinterpret_cast<T*>(to + CANARY_SIZE);

    for (size_t item_id = 0; item_id < item_count_; ++item_id, ++cur_item, ++new_item) {
      *new_item = *cur_item;
    }
#ifndef NDEBUG
    std::cerr << "copy items end\n";
#endif
  }

  void setCapacity(size_t new_capacity) {
    ASSERT_CORRECTNESS();
    if (capacity_ == new_capacity) {
      return;
    }

    char* new_bytes = new char[new_capacity * sizeof(T) + 2 * CANARY_SIZE + 1];

    copyItems(bytes_, new_bytes);
    destroy();
    setBytesPtr(new_bytes);
    items_begin_ = bytes_ + CANARY_SIZE;
    capacity_ = new_capacity;

#ifndef NDEBUG
    initCanaries();
#endif
  }

  void extend() {
    ASSERT_CORRECTNESS();
    setCapacity(capacity_ * 2);
    CALC_HASHES();
  }

  void shrink() {
    ASSERT_CORRECTNESS();
    setCapacity(capacity_ / 2);
    CALC_HASHES();
  }

  void destroy() {
    if (!bytes_) {
      return;
    }
    ASSERT_CORRECTNESS();
#ifndef NDEBUG
    std::cerr << "try to destroy old items\n";
#endif
    for (size_t item_id = 0; item_id < item_count_; ++item_id) {
      getElementPtr(item_id)->~T();
    }
    delete[] bytes_;
  }

  template <class StackRef>
  void copyParameters(const StackRef&& another) {
    another.assertCorrectness(__PRETTY_FUNCTION__);
    capacity_ = another.capacity_;
    item_count_ = another.item_count_;
#ifndef NDEBUG
    initCanaries();
    hasher_ = another.hasher_;
    CALC_HASHES();
#endif
  }

#ifndef NDEBUG
  bool checkCanaries() const {
    ASSERT_PARAMS();
    ASSERT_POINTERS();
    bool all_fine = true;

    std::cerr << "\n";
    std::cerr << "main canary " << ' ' << main_canary_ << '\n';
    if (main_canary_ == CANARY_INIT_VALUE) {
      std::cerr << "OK\n";
    } else {
      std::cerr << "incorrect value! should be " << CANARY_INIT_VALUE << '\n';
      all_fine = false;
    }

    std::cerr << "canary 1 = " << ' ' << *getCanaryPtr1() << '\n';
    if (*getCanaryPtr1() == CANARY_INIT_VALUE) {
      std::cerr << "OK\n";
    } else {
      std::cerr << "incorrect value! should be " << CANARY_INIT_VALUE << '\n';
      all_fine = false;
    }

    std::cerr << "canary 2 = " << ' ' << *getCanaryPtr2() << '\n';
    if (*getCanaryPtr2() == CANARY_INIT_VALUE) {
      std::cerr << "OK\n";
    } else {
      std::cerr << "incorrect value! should be " << CANARY_INIT_VALUE << '\n';
      all_fine = false;
    }
    return all_fine;
  }

  bool checkHashSum() const {
    ASSERT_POINTERS();

    size_t correct_hash_sum = calcHashSum();
    size_t correct_full_hash = calcFullHash();
    bool ok = true;

    std::cerr << "\n";
    std::cerr << "saved hash sum: " << hash_sum_ << '\n';
    if (hash_sum_ == correct_hash_sum) {
      std::cerr << "OK\n";
    } else {
      std::cerr << "incorrect value! should be " << correct_hash_sum << '\n';
      ok = false;
    }
    std::cerr << "saved full hash: " << full_hash_ << '\n';
    if (full_hash_ == correct_full_hash) {
      std::cerr << "OK\n";
    } else {
      std::cerr << "incorrect value! should be " << correct_full_hash << '\n';
      ok = false;
    }
    return ok;
  }

  bool checkPointers() const {
    bool ok = true;

    std::cerr << "begin of stack address: " << reinterpret_cast<void*>(bytes_) << '\n';
    std::cerr << "copy of begin of stack address: " << reinterpret_cast<void*>(bytes_copy_) << '\n';
    if (bytes_ != bytes_copy_) {
      std::cerr << "copy doesn't match\n";
      ok = false;
    }
    std::cerr << "first item address: " << reinterpret_cast<void*>(items_begin_) << '\n';
    if (items_begin_ != bytes_ + CANARY_SIZE) {
      std::cerr << "begin of stack address and first item address are not consistent to each other\n";
      ok = false;
    }
    if (capacity_ < 8 || (capacity_ & (capacity_ - 1)) != 0) {
      std::cerr << "invalid value of capacity\n";
      std::cerr << "capacity: " <<  capacity_ << "\n";
      std::cerr << "it is not a power of two that more than 4\n";
      ok = false;
    }
    if (item_count_ > capacity_ * MAX_LOAD_FACTOR) {
      std::cerr << "saved number of elements in stack is too large\n";
      std::cerr << "item count: " <<  item_count_ << "\n";
      std::cerr << "max possible item count: " << static_cast<int>(capacity_ * MAX_LOAD_FACTOR) << "\n";
      ok = false;
    }
    return ok;
  }

  bool checkParams() const {
    try {
      assertParams();
      return true;
    } catch (StackException&) {
      return false;
    }
  }

  void printItems() const {
    ASSERT_POINTERS();

    std::cerr << "\n";
    std::cerr << "item count: " << item_count_ << '\n';
    std::cerr << "items: ";
    if (item_count_ > 0) {
      for (size_t item_id = 0; item_id < item_count_; ++item_id) {
        std::cerr << *getElementPtr(item_id) << ' ';
      }
    }
    std::cerr << '\n';
  }

  const size_t MODULO = 1e12 + 7;
  const size_t BASE = 15487469;

  size_t calcHashSum() const {
    assertParams(__PRETTY_FUNCTION__);
    assertPointers(__PRETTY_FUNCTION__);

    size_t res = item_count_;

    for (size_t item_id = 0; item_id < item_count_; ++item_id) {
      res = (res * BASE + hasher_(*getElementPtr(item_id))) % MODULO;
    }
    return res;
  }

  size_t calcFullHash() const {
    const char* const_hack_ptr = reinterpret_cast<const char*>(this);

    size_t byte_cnt = sizeof(Stack);
    size_t res = 0;
    for (size_t byte_id = 0; byte_id < byte_cnt; ++byte_id, ++const_hack_ptr) {
      res = (res * BASE + std::hash<char>()(*const_hack_ptr)) % MODULO;
    }
    return res;
  }

  void assertParams(const std::string& func_name = "") const {
    bool correct = true;

    correct &= (EXPANSION_COEFF == 2);
    correct &= (SHRINKAGE_COEFF == 2);
    correct &= (MAX_LOAD_FACTOR == 0.5);
    correct &= (MIN_LOAD_FACTOR == 0.25);
    correct &= (MIN_CAPACITY == 8);
    correct &= (CANARY_SIZE == 4);
    correct &= (CANARY_INIT_VALUE == 1983776228);
    if (!correct) {
      throw ParamsPoisonedException("parameters are corrupted", func_name);
    }
  }

  void assertPointers(const std::string& func_name = "") const {
    if (bytes_copy_ != bytes_ || items_begin_ != bytes_ + CANARY_SIZE
      || capacity_ < 8 || item_count_ > capacity_ * MAX_LOAD_FACTOR) {
      throw IncorrectPointerException("pointers are destroyed", func_name);
    }
  }

  void assertCanaries(const std::string& func_name = "") const {
    if (main_canary_ != CANARY_INIT_VALUE) {
      throw CanaryException("problem with main canary", func_name);
    }
    if (*getCanaryPtr1() != CANARY_INIT_VALUE) {
      throw CanaryException("problem with first canary", func_name);
    }
    if (*getCanaryPtr2() != CANARY_INIT_VALUE) {
      throw CanaryException("problem with second canary", func_name);
    }
  }

  void assertHashSum(const std::string& func_name = "") const {
    if (full_hash_ != calcFullHash()) {
      throw HashSumException("main hash was crashed", func_name);
    }
    if (hash_sum_ != calcHashSum()) {
      throw HashSumException("hash sum of elements was crashed", func_name);
    }
  }

  void assertCorrectness(const std::string& func_name) const {
    try {
      assertParams(func_name);
      assertPointers(func_name);
      assertCanaries(func_name);
      assertHashSum(func_name);
    } catch (StackException& exc) {
      dump(func_name);
      std::cerr << "\n\n";
      throw exc;
    }
  }
#endif

  void setBytesPtr(char* new_bytes) {
    bytes_ = new_bytes;
#ifndef NDEBUG
    bytes_copy_ = new_bytes;
#endif
  }

  T& get(size_t pos) {
    if (pos >= item_count_) {
      throw OutOfRangeException("the argument is more than the number of elements", __PRETTY_FUNCTION__);
    }
#ifndef NDEBUG
    assertCorrectness();
#endif
    return *getElementPtr(pos);
  }

 public:
  Stack() {
    setBytesPtr(new char[MIN_CAPACITY * sizeof(T) + 2 * CANARY_SIZE + 1]);
    items_begin_ = bytes_ + CANARY_SIZE;
    capacity_ = MIN_CAPACITY;
#ifndef NDEBUG
    initCanaries();
    CALC_HASHES();
    std::cerr << "stack was created\n";
#endif
  }

  Stack(size_t size, const T& value): Stack() {
    size_t optimal_capacity = capacity_;

    while (optimal_capacity * MAX_LOAD_FACTOR < size) {
      optimal_capacity *= 2;
    }
    setCapacity(optimal_capacity);
    for (size_t item_id = 0; item_id < item_count_; ++item_id) {
      *getElementPtr(item_id) = value;
    }
    CALC_HASHES();
  }

  Stack(Stack&& another) {
#ifndef NDEBUG
    another.assertCorrectness();
#endif
    setBytesPtr(another.bytes_);
    items_begin_ = bytes_ + CANARY_SIZE;
    copyParameters(std::move(another));
    another.setBytesPtr(nullptr);
    CALC_HASHES();
  }

  Stack(const Stack& another) {
#ifndef NDEBUG
    another.assertCorrectness(__PRETTY_FUNCTION__);
#endif
    setBytesPtr(new char[2 * CANARY_SIZE + another.capacity_ * sizeof(T) + 1]);
    items_begin_ = bytes_ + CANARY_SIZE;
    std::cerr << "copy par\n";
    copyParameters(std::forward<const Stack>(another));
    std::cerr << "copy par finished\n";
    std::cerr << item_count_ << ' ' << another.item_count_ << '\n';
    copyItems(another.bytes_, bytes_);
    CALC_HASHES();
  }

  size_t size() const {
    ASSERT_CORRECTNESS();
    return item_count_;
  }

  bool empty() const {
    ASSERT_CORRECTNESS();
    return item_count_ > 0;
  }

  T& operator=(Stack&& another) {
    if (this == &another) {
      return *this;
    }
#ifndef NDEBUG
    assertCorrectness();
    another.assertCorrectness();
#endif
    destroy();
    setBytesPtr(another.bytes_);
    items_begin_ = bytes_ + CANARY_SIZE;
    copyParameters(another);
    another.setBytesPtr(nullptr);
    CALC_HASHES();
    return *this;
  }

  T& operator=(const Stack& another) {
    if (this == &another) {
      return *this;
    }
#ifndef NDEBUG
    assertCorrectness();
    another.assertCorrectness();
#endif
    destroy();
    std::cerr << "old one was destroyed\n";
    setBytesPtr(new char[2 * CANARY_SIZE + another.capacity_ * sizeof(T) + 1]);
    items_begin_ = bytes_ + CANARY_SIZE;
    std::cerr << "pointers were set\n";
    copyParameters(another);
    copyItems(another.bytes_, bytes_);
    CALC_HASHES();
    return *this;
  }

  const T& operator[](size_t pos) const {
    if (pos >= item_count_) {
      throw OutOfRangeException("the argument is more than the number of elements", __PRETTY_FUNCTION__);
    }
    ASSERT_CORRECTNESS();
    return *getElementPtr(pos);
  }

  void push(const T& value) {
    ASSERT_CORRECTNESS();
    if ((item_count_ + 1) > MAX_LOAD_FACTOR * capacity_) {
      extend();
    }
    *getElementPtr(item_count_) = value;
    ++item_count_;
    CALC_HASHES();
  }

  void push(T&& value) {
    ASSERT_CORRECTNESS();
    if ((item_count_ + 1) > MAX_LOAD_FACTOR * capacity_) {
      extend();
    }
    *getElementPtr(item_count_) = std::move(value);
    ++item_count_;
    CALC_HASHES();
  }

  void pop() {
    ASSERT_CORRECTNESS();
    if (item_count_ == 0) {
      throw EmptyStackException("", __PRETTY_FUNCTION__);
    }
    if (capacity_ > MIN_CAPACITY && (item_count_ - 1) < MIN_LOAD_FACTOR * capacity_) {
      shrink();
    }
    --item_count_;
    CALC_HASHES();
  }

  T top() {
    ASSERT_CORRECTNESS();
    return get(item_count_ - 1);
  }

  T extract() {
    T result = get(item_count_ - 1);

    pop();
    return result;
  }

#ifndef NDEBUG
  bool check() const {
    try {
      assertParams();
      assertPointers();
      assertCanaries();
      assertHashSum();
      return true;
    } catch (StackException&) {
      return false;
    }
  }

  void dump(const std::string& func_name) const {
    std::cerr << "__________________________________________\n";
    std::cerr << "dump was called from: " << func_name << "\n\n";
    bool correct_params = checkParams();
    bool correct_pointers = checkPointers();
    bool correct_canaries = (correct_pointers ? checkCanaries() : true);
    if (!correct_pointers) {
      std::cerr << "Pointer to the begin of stack was destroyed! I'm unable to print elements of the stack.\n";
    } else {
      printItems();
    }
    bool correct_hash_sum = (correct_pointers ? checkHashSum() : true);
    bool total_correct = correct_params && correct_pointers && correct_canaries && correct_hash_sum;

    if (total_correct) {
      std::cerr << "\nALL RIGHT.\n";
      return;
    }

    std::cerr << "\n!!!A PROBLEM OCCURED!!!\n";
    if (!correct_params) {
      std::cerr << "\nconst parameters were changed through a hack\n";
    }
    if (!correct_pointers) {
      std::cerr << "\npointers were destroyed\n";
    }
    if (!correct_canaries) {
      std::cerr << "\ncanaries were overwritten\n";
    }
    if (!correct_hash_sum) {
      std::cerr << "\ncontrol hash sum don't match\n";
    }
  }
#endif

  void printStack() const {
    std::cout << "items: ";
    if (item_count_ > 0) {
      for (size_t item_id = 0; item_id < item_count_; ++item_id) {
        std::cout << *getElementPtr(item_id) << ' ';
      }
    }
    std::cout << '\n';
  }

  ~Stack() {
#ifndef NDEBUG
    if (!check()) {
      return;
    }
#endif
    destroy();
  }
};

#endif //DED_PROG_LANG_HEADER_H
