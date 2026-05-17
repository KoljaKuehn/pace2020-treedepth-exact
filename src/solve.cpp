#include <cassert>

#include "graph.hpp"
#include "utils.hpp"
#include "bitset.hpp"
#include "preprocessor.hpp"
#include "ms_solve.hpp"

using namespace sms;

// Connected case: apply preprocessing and solve.
static bool SolveDecisionConnected(const SparseGraph& graph, int k) {
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
  return mss.Solve(k);
}

// Returns true iff the treedepth of graph is at most k.
// The graph must have at most BITS (64) vertices and k must be >= 1
// (td(G) <= 0 only holds for the empty graph, which is not a meaningful query).
bool SolveDecision(const SparseGraph& graph, int k) {
  assert(graph.n() <= BITS);
  assert(k >= 1);

  // td(G) = max over connected components, so solve each independently.
  const auto components = graph.Components({});
  if ((int)components.size() > 1) {
    for (const auto& comp_verts : components) {
      // Re-index component vertices to 0..size-1 and build a subgraph.
      std::vector<int> old_to_new(graph.n(), -1);
      int idx = 0;
      for (int v : comp_verts) old_to_new[v] = idx++;
      SparseGraph comp((int)comp_verts.size());
      for (int v : comp_verts) {
        for (int u : graph.Neighbors(v)) {
          if (old_to_new[v] < old_to_new[u])
            comp.AddEdge(old_to_new[v], old_to_new[u]);
        }
      }
      if (!SolveDecision(comp, k)) return false;
    }
    return true;
  }

  return SolveDecisionConnected(graph, k);
}