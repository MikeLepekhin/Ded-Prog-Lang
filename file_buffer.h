//
// Created by mike on 16.12.18.
//

#ifndef DED_PROG_LANG_FILE_BUFFER_H
#define DED_PROG_LANG_FILE_BUFFER_H

const size_t BUF_SIZE = 1 << 16;

class FileBuffer {
 private:
  char buf_[BUF_SIZE];
  size_t buf_size_{0};
  char* buf_ptr_{nullptr};

 public:

  FileBuffer(FILE* binary_file) {
    buf_size_ = fread((void*)buf_, sizeof(char), BUF_SIZE - 1, binary_file);
    buf_ptr_ = buf_;
    std::cout << "file size: " << buf_size_ << " bytes\n";
  }

  template<class Data>
  Data readFromBuffer() {
    Data result = *reinterpret_cast<Data*>(buf_ptr_);
    buf_ptr_ += sizeof(Data);
    return result;
  }

  bool done() const {
    return buf_ptr_ == buf_ + buf_size_;
  }

};

#endif //DED_PROG_LANG_FILE_BUFFER_H
