#ifndef CAVS_MIDEND_DEP_GRAPH_H_
#define CAVS_MIDEND_DEP_GRAPH_H_

#include "cavs/midend/node.h"
#include "cavs/midend/edge.h"
#include "cavs/midend/statement.h"
#include "cavs/proto/op_def.pb.h"
#include "cavs/util/logging.h"

#include <vector>
#include <unordered_map>
#include <string>

namespace midend {

//class Node;
//class NodeGroup;
//class Edge;

//the storage of actual data
class DepGraph {
 public:
  DepGraph(const Scope* s = GetGlobalScope())
    : s_(s) {}
  inline Node* AddNode(const OpDef& op_def) {
    return s_->AddNode(); 
  }
  inline int num_nodes() const {
    return nodes_.size();
  }
  inline const Node* operator[](int node_id) const {
    return nodes_[node_id];
  }
  void GroupAllVariables(std::vector<std::string>* vars);
  void OptimizeWithLoss(const std::string& loss, 
      const std::string& solver, 
      const std::vector<std::string>& var_names);
  void Dump();

 private:
  const Scope* s_;
  //std::vector<Node*> nodes_;
  //std::vector<std::vector<Node*>> grad_nodes_;
  //std::vector<Node*> update_nodes_;
  //std::vector<Edge*> edges_;
  //void AddGradNode(const OpDef& op_def,
      //const Scope* s);
  void BackPropagate();
  void AddSolver(const std::string& solver,
      const std::vector<std::string>& vars,
      std::vector<Statement*>* stmts);
  bool SearchCriticalPath(Scope*s,
      const Edge* var, const Edge* curr,
      const unordered_map<Node*, bool>& recalculate,
      const unordered_map<Node*, bool>& accessed) {
  void SearchClosedSet(
      const std::vector<std::string>& vars,
      const std::vector<std::string>& grad_vars,
      Scope* s);
};

//class NodeGroup {
 //public:
  //explicit NodeGroup(std::string name) : name_(name) {}
  //void AddNode(const Node* n);
 //private:
  //std::vector<const Node*> nodes_;
  //std::vector<Edge*> inputs_;
  //std::vector<Edge*> outputs_;
  //std::string name_;
//};

} //namespace midend 

#endif
