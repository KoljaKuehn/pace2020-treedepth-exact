#pragma once

#include <vector>

#include "graph.hpp"

namespace sms {
class Preprocessor {
 public:
  SparseGraph Preprocess(SparseGraph graph);
  SparseGraph TamakiRules(SparseGraph graph, int k);
 private:
  void ParseTrees(SparseGraph& graph);
  std::vector<int> SolveTree(int v, const SparseGraph& graph, const std::vector<int>& parent);
};
} // namespace sms