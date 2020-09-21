#include "cavs/frontend/cxx/sym.h"
#include "cavs/frontend/cxx/graphsupport.h"
#include "cavs/frontend/cxx/session.h"
#include "cavs/proto/opt.pb.h"

#include <iostream>
#include <fstream>
#include <vector>

#include "common.h"

using namespace std;

DEFINE_int32(batch_size, 10, "batch");
DEFINE_int32(vocab_size, 21701, "input size");
DEFINE_int32(hidden_size, 256, "hidden size");
DEFINE_int32(max_batches, 1, "iterations");
DEFINE_double(init_scale, 0.1f, "init random scale of variables");
DEFINE_string(input_file, "", "input sentences");
DEFINE_string(graph_file, "", "graph dependency");

DEFINE_validator(input_file, &IsNonEmptyMessage);
DEFINE_validator(graph_file, &IsNonEmptyMessage);

class TreeModel : public GraphSupport {
 public:
  TreeModel(const Sym& graph_ph, const Sym& vertex_ph) :
    GraphSupport(graph_ph, vertex_ph) {
    embedding = Sym::Variable(DT_FLOAT, {FLAGS_vocab_size, FLAGS_hidden_size},
                            Sym::Uniform(-FLAGS_init_scale, FLAGS_init_scale));

    Wi = Sym::Variable(DT_FLOAT, {3 * FLAGS_hidden_size * FLAGS_hidden_size},
		       Sym::Uniform(-FLAGS_init_scale, FLAGS_init_scale));

    Whr = Sym::Variable(DT_FLOAT, {FLAGS_hidden_size * FLAGS_hidden_size},
		       Sym::Uniform(-FLAGS_init_scale, FLAGS_init_scale));
    Whh = Sym::Variable(DT_FLOAT, {FLAGS_hidden_size * FLAGS_hidden_size},
		       Sym::Uniform(-FLAGS_init_scale, FLAGS_init_scale));
    Whz = Sym::Variable(DT_FLOAT, {FLAGS_hidden_size * FLAGS_hidden_size},
		       Sym::Uniform(-FLAGS_init_scale, FLAGS_init_scale));
    B = Sym::Variable(DT_FLOAT, {3 * FLAGS_hidden_size}, Sym::Zeros());
    One = Sym::Constant(DT_FLOAT, 1.0f, {FLAGS_hidden_size});

    // prepare parameter symbols
    Br = B.Slice(0, FLAGS_hidden_size);
    Bh = B.Slice(FLAGS_hidden_size, FLAGS_hidden_size);
    Bz = B.Slice(2 * FLAGS_hidden_size, FLAGS_hidden_size);
  }

  void Node() override {
    Sym h_l = Gather(0, {FLAGS_hidden_size});
    Sym h_r = Gather(1, {FLAGS_hidden_size});
    Sym htm1 = h_l + h_r;

    // Pull the input word
    Sym x = Pull(0, {1});
    Sym input = x.EmbeddingLookup(embedding.Mirror());

    Sym Xi = Sym::MatMul(input.Reshape({1, FLAGS_hidden_size}).Mirror(),
			 Wi.Reshape({FLAGS_hidden_size, 3 * FLAGS_hidden_size}).Mirror())
      .Reshape({FLAGS_hidden_size * 3});
    Sym Xr, Xz, Xh;
    tie(Xr, Xz, Xh) = Xi.Split3();


    // Sym r = htm1 * (Br + Xr +
    // 		    Sym::MatMul(Whr.Reshape({FLAGS_hidden_size, FLAGS_hidden_size}).Mirror(),
    // 				htm1.Reshape({FLAGS_hidden_size, 1}).Mirror())
    // 		    .Reshape({FLAGS_hidden_size})).Sigmoid();
    // Sym z = (Bz + Xz +
    // 	     Sym::MatMul(Whz.Reshape({FLAGS_hidden_size, FLAGS_hidden_size}).Mirror(),
    // 			 htm1.Reshape({FLAGS_hidden_size, 1}).Mirror())
    // 	     .Reshape({FLAGS_hidden_size})).Sigmoid();
    // Sym h = (Bh + Xh + Sym::MatMul(Whh.Reshape({FLAGS_hidden_size, FLAGS_hidden_size}).Mirror(),
    // 				   r.Reshape({FLAGS_hidden_size, 1}).Mirror())
    // 	     .Reshape({FLAGS_hidden_size})).Tanh();
    // Sym ht = ((One - z) * h + z * htm1).Reshape({FLAGS_hidden_size});

    // Sym ht = htm1 + (Xr + Xh + Xz).Reshape({FLAGS_hidden_size}).Mirror();

    Sym r = (Br + Xr +
		    Sym::MatMul(Whr.Reshape({FLAGS_hidden_size, FLAGS_hidden_size}).Mirror(),
				htm1.Reshape({FLAGS_hidden_size, 1}).Mirror())
	     .Reshape({FLAGS_hidden_size})).Sigmoid();

    Sym ht = r.Reshape({FLAGS_hidden_size}).Mirror();

    Scatter(ht.Mirror());
    Push(ht.Mirror());
  }

 private:
  Sym Wi, Whr, Whh, Whz, B;
  Sym embedding;
  Sym Br, Bh, Bz;
  Sym One;
};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  SSTReader sst_reader(FLAGS_input_file, FLAGS_graph_file);

  Sym graph    = Sym::Placeholder(DT_FLOAT, {FLAGS_batch_size, SST_MAX_DEPENDENCY}, "CPU");
  Sym word_idx = Sym::Placeholder(DT_FLOAT, {FLAGS_batch_size, SST_MAX_DEPENDENCY});

  Sym weight   = Sym::Variable(DT_FLOAT, {FLAGS_vocab_size, FLAGS_hidden_size},
                               Sym::Uniform(-FLAGS_init_scale, FLAGS_init_scale));
  Sym bias     = Sym::Variable(DT_FLOAT, {1, FLAGS_vocab_size}, Sym::Zeros());
  TreeModel model(graph, word_idx);
  Sym graph_output = model.Output();
  Session sess(OPT_BATCHING + OPT_FUSION);
  // Session sess(OPT_BATCHING + OPT_FUSION + OPT_STREAMMING);
  int max_batches = FLAGS_max_batches;
  vector<float> input_data(FLAGS_batch_size*SST_MAX_DEPENDENCY, -1);
  vector<int>   graph_data(FLAGS_batch_size*SST_MAX_DEPENDENCY, -1);

  std::cout << "[CONFIG] " << FLAGS_batch_size << " " << FLAGS_hidden_size << std::endl;

  CHECK(FLAGS_batch_size * FLAGS_max_batches <= SST_NUM_SAMPLES);

  float all_time = 0.0;
  int num_nodes = 0;
  for (int j = 0; j < max_batches; j++) {
    int this_num_nodes = 0;
    sst_reader.next_batch(FLAGS_batch_size, &graph_data, &input_data, &this_num_nodes);
    num_nodes += this_num_nodes;

    auto runner = [&] {
      time_point<system_clock> start = system_clock::now();

      sess.Run({graph_output}, {{graph, graph_data.data()},
	                        {word_idx,input_data.data()}});

      time_point<system_clock> end = system_clock::now();
      std::chrono::duration<float> fs = (end - start);
      return duration_cast<microseconds>(fs).count();
    };
    all_time += measure_time(runner);
  }

  report_time(all_time, num_nodes, max_batches);

  return 0;
}
