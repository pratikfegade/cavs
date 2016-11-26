#ifndef CAVS_CORE_TENSOR_H_
#define CAVS_CORE_TENSOR_H_

#include "cavs/core/types.pb.h"
#include "cavs/core/macros.h"
#include "cavs/core/allocator.h"

#include <vector>
#include <string>
#include <memory>

using std::vector;
using std::string;

namespace cavs {

class TensorBufferBase {
 public:
  TensorBufferBase(Allocator* alloc) : alloc_(alloc) {}
  virtual ~TensorBufferBase() {}
  virtual void* data() const = 0;
  //virtual size_t size() const = 0;
  virtual size_t count() const = 0;

 protected:
  Allocator* const alloc_;
};

class TensorShape {
 public:
  TensorShape() : n_elements_(0) {}
  TensorShape(vector<int>& shape);
  TensorShape(std::initializer_list<int> shape);
  void operator = (const TensorShape& b);
  void set_dim(int d, int size);
  void add_dim(int size);
  int n_elements() { return n_elements_; }
  int dims() { return shape_.size(); }
 private:
  vector<int> shape_;
  int n_elements_;
};

FORCE_INLINE void TensorShape::operator = (const TensorShape& b) {
  n_elements_ = b.n_elements_;
  shape_ = b.shape_;
}

class Tensor {
 public:
  Tensor() {}
  //Tensor(DataType type, const std::vector<int>& shape_);
  //Tensor(DataType type, std::initializer_list<int> shape_);
  //Tensor(Allocator *a, DataType type, std::initializer_list<int> shape);
  Tensor(const string& name, Allocator *a, DataType type, const TensorShape& shape);
  void Reshape(Allocator *a, DataType type, const TensorShape& shape);
  template <typename T>
    T* mutable_data() const { return reinterpret_cast<T*>(buf_->data()); }
  template <typename T>
    const T* data() const { return reinterpret_cast<T*>(buf_->data()); }
  size_t count() const { return buf_->count(); }
  string name() const { return name_; }
  const TensorShape& shape() const { return shape_; }

 private:
  std::unique_ptr<TensorBufferBase> buf_;
  TensorShape shape_;
  string name_;
};

} //namespace cavs

#endif

