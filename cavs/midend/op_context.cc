#include "cavs/midend/op_context.h"

#include <string>

using std::string;

namespace midend {

string OpContext::DebugInfo() {
  string info;
  for (unsigned i = 0; i < inputs_.size(); i++) {
    info += "input tensor[" + std::to_string(i)
            + "]:\t" + inputs_[i].name();
    info += "\n";
  }
  for (unsigned i = 0; i < outputs_.size(); i++) {
    info += "output tensor[" + std::to_string(i)
            + "]:\t" + outputs_[i].name();
    info += "\n";
  }
  return info;
}

} //namespace midend