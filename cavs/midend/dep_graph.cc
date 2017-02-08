#include "cavs/midend/dep_graph.h"
#include "cavs/midend/statement_builder.h"
#include "cavs/util/logging.h"
#include "cavs/backend/op_decl.h"
#include "cavs/backend/op_def_builder.h"

#include <string>
#include <algorithm>
#include <list>

using namespace std;
using ::backend::OpDecl;
using ::backend::BuildConstantOpDef;

namespace midend {

Node* DepGraph::AddNode(const OpDef& op_def) { 
  return const_cast<Scope*>(s_)->AddNode(op_def); 
}

int DepGraph::num_nodes() const {
  return s_->nodes_.size();
}

const Node* DepGraph::operator[](int node_id) const {
  return s_->nodes_[node_id];
}

bool DepGraph::TraverseCriticalPath(Scope*s,
      const Edge* loss, const Edge* curr,
      unordered_map<const Node*, bool>* fwd_path,
      list<const Node*>* newly_traversed) {
  CHECK(curr->srcs_size() == 1);
  CHECK(curr->dsts_size() == 1);
  const Node* node = curr->dst(0);
  if (curr == loss ||
      (fwd_path->find(node) != fwd_path->end() &&
       (*fwd_path)[node])) {
    for (auto* node : *newly_traversed)
      s->AddNode(node->op_def());
    newly_traversed->clear();
    return true;
  }
  if (fwd_path->find(node) == fwd_path->end() ||
      !(*fwd_path)[node]) {
    (*fwd_path)[node] = false;
    newly_traversed->push_back(node);
    bool in_path = false;
    for (auto* edge : node->outputs()) {
      if (TraverseCriticalPath(s, loss, edge, fwd_path, newly_traversed)) {
        const vector<OpDef>& grads = 
          ::backend::MakeGradient(node->op_def()); 
        for (auto& grad : grads) {
          if (std::find(grad.output().begin(), grad.output().end(),
                        curr->name()) == grad.output().end())
            continue;
          Node* grad_node = const_cast<Scope*>(s_)->AddNode(grad);
          vector<TensorShapeDef> inputs;
          grad_node->InputShapes(&inputs);
          const vector<TensorShapeDef>& shapes = 
            ::backend::ShapeInference(grad, inputs);
          grad_node->SetShape(shapes);
          in_path = true;
          (*fwd_path)[node] = true; 
        }
      }
    }
    if (!in_path)
      newly_traversed->pop_back();
  }
  return (*fwd_path)[node];
}

void DepGraph::SearchClosedSet(
    const vector<string>& vars,
    const vector<string>& grad_vars,
    Scope* s) {
  CHECK(vars.size() == grad_vars.size());
  unordered_map<const Node*, bool> recalculate;
  for (int i = 0; i < vars.size(); i++) {
    const Edge* var = s->FindEdge(vars[i]);
    const Edge* grad_var = s->FindEdge(grad_vars[i]);
    list<const Node*> newly_traversed;
    TraverseCriticalPath(s, var, grad_var,
        &recalculate, &newly_traversed);
  }
}

void DepGraph::GroupAllVariables(vector<string>* vars) {
  for (Node* n : s_->nodes_) {
    if (n->IsVariableOp())
      vars->push_back(n->name());
  }
}

void DepGraph::OptimizeWithLoss(
    const string& loss, 
    const string& solver, 
    const vector<string>& var_names) {
  CHECK(var_names.size() > 0);
  Scope* s_loss = new Scope(GetGlobalScope(), loss);
  vector<string> grad_var_names;
  for (auto& var : var_names)
    grad_var_names.push_back(OpDecl::GetGradientName(var));
  Edge* loss_edge = s_->FindEdge(loss);
  CHECK(loss_edge);
  OpDef const_op;
  BuildConstantOpDef(&const_op, 
      OpDecl::GetGradientName(loss),
      loss_edge->shape(),
      1.f);
  s_loss->AddNode(const_op);
  SearchClosedSet(var_names, grad_var_names, s_loss);
  
  //for (auto& var : var_names) {
    //const string& var_grad = 
      //OpDecl::GetGradientName(var);
  //}
}

//currently, we assume all ops defined by developpers
//are in the scope of global
void DepGraph::BackPropagate() {
  //for (int i = grad_nodes_.size();
        //i < nodes_.size(); i++) {
    //AddGradNode(nodes_[i]->op_def(), GetGlobalScope());  
  //}
  //////gen_grads->clear();
  //for (int i = num_nodes()-1; i >= 0; i--) {
    //const vector<OpDef>& grads = 
      //::backend::MakeGradient(nodes_[i]->op_def()); 
    //for (auto& grad : grads) {
      //////for (auto& grad_out_str : grad.output())
        //////gen_grads->push_back(grad_out_str);
      ////for (auto& grad_input : grad.input()) {
        //////if the grad_input does not exist, 
        //////it must be the loss node,
        //////and it should be set to one-value matrix
        ////if (out2edge_.find(grad_input) == out2edge_.end()) {
          ////const string& ori_input = 
            ////::backend::OpDecl::GetOriginName(grad_input);
          ////CHECK(out2edge_.find(ori_input) != out2edge_.end());
          ////OpDef const_op;
          ////::backend::BuildConstantOpDef(&const_op, 
              ////grad_input,
              ////out2edge_[ori_input]->tensor_shape_,
              ////1.f);
          ////AddNode(const_op);
        ////}
      ////}
      //Node* node = AddNode(grad);
      //vector<TensorShapeDef> inputs;
      //node->InputShapes(&inputs);
      //const vector<TensorShapeDef>& out_shapes = 
        //::backend::ShapeInference(grad, inputs);
      //node->SetShape(out_shapes);
    //}
  //}
}

void DepGraph::AddSolver(
    const string& solver,
    const vector<string>& var_names,
    vector<Statement*>* stmts) {
  //for (auto& var : var_names) {
    //CHECK(out2edge_.find(var) != out2edge_.end());
    //const string& var_grad = 
      //::backend::OpDecl::GetGradientName(var);
    //CHECK(out2edge_.find(var_grad) != out2edge_.end());
    //OpDef update;  
    //::backend::OpDefBuilder(solver)
        //.Input(var_grad)
        //.Output(var)
        //.Shape(out2edge_.at(var)->tensor_shape_)
        //.Finalize(&update);
    ////Node* node = AddNode(update);
    //stmts->emplace_back(BuildExprStatement(update));
  //}
}

//void DepGraph::SetLossNodeGroup(const string& loss,
    //const vector<string>& vars,
    //const Scope* s) {
  ////CHECK(s.Fine(loss) == out2ng_.end());
  ////NodeGroup* ng = new NodeGroup(loss); 
//}

void DepGraph::Dump() {
  for (auto* node : s_->nodes_)
    LOG(INFO) << node->op_def().DebugString();
}

} //namespace midend
