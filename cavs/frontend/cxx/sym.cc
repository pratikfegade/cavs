#include "cavs/frontend/cxx/sym.h"
#include "cavs/frontend/c_api.h"
#include "cavs/proto/devices.pb.h"
#include "cavs/util/logging.h"

using std::vector;

void Sym::node::Finalize(OpDef* op_def) const {
  op_def->set_name(op_name_);
  for (const string& str: input_)
    op_def->add_input(str);
  for (const string& str: output_)
    op_def->add_output(str);
  op_def->set_dtype(DataType((int)type_));
  //device
  if (device_ == "GPU")
    op_def->set_device(GPU);
  else
    op_def->set_device(CPU);
  //shape
  op_def->clear_shape();
  TensorShapeDef* shape_def = op_def->add_shape();
  for (auto dim : shape_)
    shape_def->add_dim(dim);
}

Sym::Sym(const string& op_name,
         const vector<string>& inputs, 
         const C_Dtype type,
         const string& device,
         const vector<int>& shape,
         const vector<OpDef::AttrDef>& attrs) {
  static int id = 0;  
  node_.reset(new node());
  node_->op_name_ = op_name;
  node_->output_.push_back(op_name + std::to_string(id++));
  node_->input_ = inputs;
  node_->type_ = type;
  node_->shape_ = shape; 
  node_->device_ = device; 

  OpDef op_def;
  node_->Finalize(&op_def);
  for (auto& attr : attrs)
    *(op_def.add_attr()) = attr;
  string serial_def;
  op_def.SerializeToString(&serial_def);
  int *dim = NULL;
  size_t dim_length;
  C_AddNode(C_GetDefaultDG(),
      serial_def.c_str(), serial_def.length(),
      &dim, &dim_length);
  node_->shape_.clear();
  for (int i = 0; i < dim_length; i++)
    node_->shape_.push_back(dim[i]);
  free(dim);
}

Sym::Sym(const string& op_name,
    const string& loss,
    const vector<Sym>& variables,
    const int iters,
    const string& projection) {
  CHECK(op_name == "Optimizer");
  static int id = 0;
  node_.reset(new node());
  node_->op_name_ = op_name;
  node_->output_.push_back(op_name + std::to_string(id++));
  node_->input_ = {loss};

  OpDef op_def;
  node_->Finalize(&op_def);

  if (variables.size()) {
    OpDef::AttrDef* var_attr = op_def.add_attr();
    var_attr->set_name("vars");
    OpDef::AttrType::ListValue* str_list
      = var_attr->mutable_value()->mutable_list();
    for (auto& sym : variables)
      str_list->add_s(sym.output(0));
  }
  OpDef::AttrDef* solver_attr = op_def.add_attr();
  solver_attr->set_name("solver");
  solver_attr->mutable_value()->set_s("SGD");
  OpDef::AttrDef* proj_attr = op_def.add_attr();
  proj_attr->set_name("projection");
  proj_attr->mutable_value()->set_s(projection);
  OpDef::AttrDef* iters_attr = op_def.add_attr();
  iters_attr->set_name("iters");
  iters_attr->mutable_value()->set_i(iters);

  string serial_def;
  op_def.SerializeToString(&serial_def);
  C_OptimizeWithLoss(C_GetDefaultDG(),
    serial_def.c_str(), serial_def.length());
}

Sym Sym::Variable(C_Dtype type, vector<int> shape,
    const OpDef::AttrDef& attr, string device) {
  CHECK(shape.size() > 0);
  return Sym("Variable", {}, type, device, shape, {attr});
}

Sym Sym::Placeholder(C_Dtype type, vector<int> shape, string device) {
  CHECK(shape.size() > 0);
  return Sym("Placeholder", {}, type, device, shape);
}

Sym Sym::MnistInput(int batch, string file, string device) {
  OpDef::AttrDef attr;
  attr.set_name("Batch");
  attr.mutable_value()->set_i(batch);
  return Sym("MnistInput", {}, C_FLOAT, device, {}, {attr}); 
}

Sym Sym::Abs(const Sym& a, string device) {
  CHECK(a.node_->output_.size() == 1);
  Sym s("Abs", {a.node_->output_[0]},
        a.node_->type_, device);
  return s;
}

Sym Sym::Square(const Sym& a, string device) {
  CHECK(a.node_->output_.size() == 1);
  Sym s("Square", {a.node_->output_[0]},
        a.node_->type_, device);
  return s;
}

Sym Sym::Maxpooling(const Sym&a,
    int HightWindow, int WidthWindow, string device) {
  vector<OpDef::AttrDef> attrs;
  {
    OpDef::AttrDef attr;
    attr.set_name("HightWindow");
    attr.mutable_value()->set_i(HightWindow);
    attrs.push_back(std::move(attr));
  }
  {
    OpDef::AttrDef attr;
    attr.set_name("WidthWindow");
    attr.mutable_value()->set_i(HightWindow);
    attrs.push_back(std::move(attr));
  }
  {
    OpDef::AttrDef attr;
    attr.set_name("PoolingMode");
    attr.mutable_value()->set_s("Max");
    attrs.push_back(std::move(attr));
  }
  return Sym("Pooling", {a.node_->output_[0]}, a.node_->type_,
         device, {}, attrs);
}

Sym Sym::Relu(const Sym&a, string device) {
  return Sym("Relu", {a.node_->output_[0]}, a.node_->type_, device);
}

Sym Sym::SoftmaxEntropyLogits(const Sym&a, string device) {
  return Sym("SoftmaxEntropyLogits", { a.node_->output_[0] },
        a.node_->type_, device);
}

Sym Sym::Flatten(const Sym& a) {
  OpDef::AttrDef attr;
  attr.set_name("ShareMemory");
  attr.mutable_value()->set_b(true);
  return Sym("Flatten", { a.node_->output_[0] },
      a.node_->type_, a.node_->device_, {}, {attr});
}

Sym Sym::Add(const Sym& a, const Sym& b, string device) {
  CHECK(a.node_->type_ == b.node_->type_);
  CHECK(a.node_->output_.size() == b.node_->output_.size() == 1);
  //Sym s("Add", output, {a.output(), b.output()}, a.type(), device, a.shape());
  Sym s("Add", {a.node_->output_[0], b.node_->output_[0]},
        a.node_->type_, device);
  return s;
}

Sym Sym::Sub(const Sym& a, const Sym& b, string device) {
  CHECK(a.node_->type_ == b.node_->type_);
  CHECK(a.node_->output_.size() == b.node_->output_.size() == 1);
  Sym s("Sub", {a.node_->output_[0], b.node_->output_[0]},
        a.node_->type_, device);
  return s;
}

Sym Sym::Mul(const Sym& a, const Sym& b, string device) {
  CHECK(a.node_->type_ == b.node_->type_);
  CHECK(a.node_->output_.size() == b.node_->output_.size() == 1);
  Sym s("Mul", {a.node_->output_[0], b.node_->output_[0]},
        a.node_->type_, device);
  return s;
}

Sym Sym::MatMul(const Sym& a, const Sym& b, string device) {
  CHECK(a.node_->type_ == b.node_->type_);
  CHECK(a.node_->output_.size() == b.node_->output_.size() == 1);
  Sym s("MatMul", {a.node_->output_[0], b.node_->output_[0]},
        a.node_->type_, device);
  return s;
}

Sym Sym::Conv(const Sym& a, const Sym& b, const Sym& c, string device) {
  CHECK(b.op_name() == "Variable");
  CHECK(c.op_name() == "Variable");
  CHECK(a.node_->type_ == b.node_->type_ &&
        b.node_->type_ == c.node_->type_)
    << a.node_->type_ << b.node_->type_ << c.node_->type_;
  return Sym("Conv",
      {a.node_->output_[0], b.node_->output_[0], c.node_->output_[0]},
      a.node_->type_, device);
}

Sym Sym::FullyConnected(const Sym& a, const Sym& b, string device) {
  CHECK(b.op_name() == "Variable");
  CHECK(a.node_->type_ == b.node_->type_);
  return MatMul(a, b, device);
}

Sym Sym::Optimizer(const Sym& a) {
  CHECK(a.node_->output_.size() == 1);
  Sym s("Optimizer", a.node_->output_[0]);
  return s;
}

//filler operation
OpDef::AttrDef Sym::Ones() {
  OpDef::AttrDef attr;
  attr.set_name("ConstFiller");
  attr.mutable_value()->set_f(1.f);
  return attr;
}

Sym Sym::Optimizer(const Sym& a, vector<Sym> variables,
    int iters, const string& projections) {
  CHECK(variables.size() > 0);
  CHECK(iters > 0);
  CHECK(a.node_->output_.size() == 1);
  Sym s("Optimizer", a.node_->output_[0],
      variables, iters, projections);
  return s;
}

Sym& Sym::operator= (const Sym& sym) {
  this->node_ = sym.node_; 
  return *this;
}

void Sym::print() {
  //hack here
  int length = 1;
  CHECK_NOTNULL(node_.get());
  for (int dim : node_->shape_)
    length *= dim;
  if (node_->type_ == C_FLOAT) {
    for (int i = 0; i < length; i++)
      LOG(INFO) << (float)((float*)node_->raw_data)[i];
  }
}

void Sym::DumpGraph() {
  C_DumpGraph(C_GetDefaultDG());
}

