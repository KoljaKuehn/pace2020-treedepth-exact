#include <cassert>

#include "graph.hpp"
#include "utils.hpp"
#include "bitset.hpp"
#include "preprocessor.hpp"
#include "ms_solve.hpp"

using namespace sms;

// Returns true iff the treedepth of graph is at most k.
// The graph must have at most BITS (64) vertices.
bool SolveDecision(const SparseGraph& graph, int k) {
  assert(graph.n() <= BITS);
  Preprocessor pp;
  SparseGraph pp_graph = pp.Preprocess(graph);
  if (pp_graph.n() == 0) return true;
  assert(pp_graph.n() <= BITS);
  Preprocessor pp2 = pp;
  SparseGraph pp_graph2 = pp2.TamakiRules(pp_graph, k);
  if (pp_graph2.n() == 0) return true;
  assert(pp_graph2.n() <= BITS);
  FGraph fg(pp_graph2);
  MSSolve mss(fg);
  int result = mss.Solve(k, true);
  return result <= k;
}