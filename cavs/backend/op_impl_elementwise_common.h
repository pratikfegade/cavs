#ifndef CAVS_BACKEND_OPS_IMPL_ELEMENTWISE_COMMON_H_
#define CAVS_BACKEND_OPS_IMPL_ELEMENTWISE_COMMON_H_

#include "cavs/backend/op_impl.h"
#include "cavs/midend/allocator.h"
#include "cavs/proto/op_def.pb.h"
#include "cavs/proto/tensor_shape.pb.h"

using ::midend::Tensor;

namespace backend {

template <typename FUNCTOR, typename T>//mathop, dtype
class UnaryOp : public OpImpl {
 public:
  explicit UnaryOp(const OpDef& def) : OpImpl(def) {}
  void Compute(OpContext* context) override {
    const Tensor& inp = context->Input(0);
    //inp.DebugNumerical<T>();
    Tensor* out = context->Output(0);
    FUNCTOR::Compute(out->mutable_data<T>(), out->count(), 
        inp.data<T>(), inp.count());
    //out->DebugNumerical<T>();
  }
};

template <typename FUNCTOR, typename T>
class BinaryOp : public OpImpl {
 public:
  explicit BinaryOp(const OpDef& def) : OpImpl(def) {}
  void Compute(OpContext* context) override {
    const Tensor& inp0 = context->Input(0);
    const Tensor& inp1 = context->Input(1);
    //inp0.DebugNumerical<T>();
    //inp1.DebugNumerical<T>();
    Tensor* out = context->Output(0);
    FUNCTOR::Compute(out->mutable_data<T>(), out->count(), 
        inp0.data<T>(), inp0.count(), inp1.data<T>(), inp1.count());
    //out->DebugNumerical<T>();
  }
};

template <typename FUNCTOR, typename T>
class PartialUnaryOp : public UnaryOp<FUNCTOR, T> {
 public:
  explicit PartialUnaryOp(const OpDef& def)
    : UnaryOp<FUNCTOR, T>(def),
      split_(-1), index_(-1), offset_(-1), stride_(-1) {
    if (GetSingleArg(def, "Split", 0) != 0) {
      //dynamic slicing
      split_ = GetSingleArg<int>(def, "Split"); 
      index_ = GetSingleArg<int>(def, "Index"); 
      CHECK(split_ > 0);
      CHECK(index_ >= 0);
    }else {
      //static slicing
      offset_ = GetSingleArg<int>(def, "Offset");
      stride_ = GetSingleArg<int>(def, "Stride");
      CHECK(offset_ >= 0);
      CHECK(stride_ > 0);
    }
  }
  void Compute(OpContext* context) override {
    const Tensor& inp = context->Input(0);
    //inp.DebugNumerical<T>();
    Tensor* out = context->Output(0);

    //inp is the small tensor and out is the big one
    if (split_ > 0) {
      //it means the dynamic slicing
      CHECK(inp.count() % split_ == 0) << inp.count() << "\t" << split_;
      stride_ = inp.count() / split_;
      offset_ = stride_ * index_;
    }
    CHECK(inp.count() == stride_);
    FUNCTOR::Compute(out->mutable_data<T>()+offset_, stride_, 
        inp.data<T>(), stride_);
    //out->DebugNumerical<T>();
  }

 private:
  int offset_;
  int stride_;
  int split_;
  int index_;
};

} //namespace backend

#endif
