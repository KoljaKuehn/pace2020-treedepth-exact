#include "preprocessor.hpp"

#include "graph.hpp"
#include "utils.hpp"

#define F first
#define S second

using std::vector;
using std::queue;
using std::max;
using std::pair;

namespace sms {
namespace {
int SubtreeSize(int v, const SparseGraph& graph, const vector<int>& parent) {
  int sz = 1;
  for (int nv : graph.Neighbors(v)) {
    if (parent[nv] == v) {
      sz += SubtreeSize(nv, graph, parent);
    }
  }
  return sz;
}

void DelTree(int v, SparseGraph& graph, const vector<int>& parent, vector<int>& isols) {
  auto nbs = graph.Neighbors(v);
  for (int nv : nbs) {
    if (parent[nv] == v) {
      DelTree(nv, graph, parent, isols);
      graph.RemoveEdge(v, nv);
      assert(graph.Degree(nv) == 0);
      isols.push_back(nv);
    }
  }
}
} // namespace

vector<int> Preprocessor::SolveTree(int v, const SparseGraph& graph, const vector<int>& parent) {
  vector<int> rank = {1};
  int root = 0;
  for (int nv : graph.Neighbors(v)) {
    if (parent[nv] == v) {
      auto sr = SolveTree(nv, graph, parent);
      rank.resize(max(sr.size(), rank.size())+1);
      for (int i=root;i<(int)sr.size();i++){
        assert(sr[i] <= 1);
        rank[i] += sr[i];
      }
      for (int i=root;i<(int)rank.size();i++){
        assert(rank[i] <= 3);
        if (rank[i] >= 2) {
          assert(i+1>root);
          assert(i+1<(int)rank.size());
          root=i+1;
          rank[i] = 0;
          rank[i+1]++;
        }
        assert(rank[i] <= 1);
      }
      for (int i=0;i<root;i++) {
        rank[i] = 0;
      }
      while (rank.back() == 0){
        rank.pop_back();
      }
    }
  }
  assert(rank.size() > 0 && rank.back() == 1);
  assert(root < (int)rank.size());
  assert(rank[root] == 1);
  return rank;
}

void Preprocessor::ParseTrees(SparseGraph& graph) {
  int n = graph.n();
  vector<int> dgs(n);
  vector<int> parent(n);
  vector<int> isols;
  queue<int> proc;
  for (int i=0;i<n;i++) {
    parent[i] = -1;
    dgs[i] = graph.Degree(i);
    if (dgs[i] == 0) {
      isols.push_back(i);
    } else if (dgs[i] == 1) {
      proc.push(i);
    }
  }
  while (!proc.empty()) {
    int v = proc.front();
    proc.pop();
    int cnt = 0;
    for (int nv : graph.Neighbors(v)) {
      if (parent[nv] == -1) {
        cnt++;
      }
    }
    assert(cnt == dgs[v]);
    assert(dgs[v] <= 1);
    assert(parent[v] == -1);
    for (int nv : graph.Neighbors(v)) {
      if (parent[nv] == -1) {
        parent[v] = nv;
        dgs[nv]--;
        if (dgs[nv] == 1) {
          proc.push(nv);
        }
      } else {
        assert(parent[nv] == v);
      }
    }
  }
  for (int i=0;i<n;i++) {
    if (parent[i] == -1) {
      int sz = SubtreeSize(i, graph, parent);
      if (sz >= 3) {
        auto sol = SolveTree(i, graph, parent);
        DelTree(i, graph, parent, isols);
        int v = i;
        for (int j = 0; j < (int)sol.size(); j++) {
          assert(sol[j] >= 0 && sol[j] <= 1);
          if (sol[j] == 0) continue;
          vector<int> clq = {v};
          for (int jj = 0; jj < j; jj++) {
            assert(isols.size() > 0);
            clq.push_back(isols.back());
            isols.pop_back();
          }
          assert((int)clq.size() == j+1);
          for (int a : clq) {
            for (int b : clq) {
              if (a < b) {
                assert(!graph.HasEdge(a, b));
                graph.AddEdge(a, b);
              }
            }
          }
          if (j+1 < (int)sol.size()) {
            assert(isols.size() > 0);
            graph.AddEdge(v, isols.back());
            v = isols.back();
            isols.pop_back();
          }
        }
      }
    }
  }
}

SparseGraph Preprocessor::Preprocess(SparseGraph graph) {
  ParseTrees(graph);
  return SparseGraph(graph.Edges());
}

SparseGraph Preprocessor::TamakiRules(SparseGraph graph, int k) {
  bool fo = true;
  while (fo) {
    fo = false;
    for (int x = 0; x < graph.n(); x++) {
      if (graph.Degree(x) < k) continue;
      for (int y = x+1; y < graph.n(); y++) {
        if (graph.Degree(y) < k) continue;
        if (graph.HasEdge(x, y)) continue;
        if (graph.Mincut(x, y) >= k) {
          graph.AddEdge(x, y);
          fo = true;
        }
      }
    }
  }
  vector<pair<int, int>> dgo;
  for (int i=0;i<graph.n();i++){
    dgo.push_back({graph.Degree(i), i});
  }
  sort(dgo.begin(), dgo.end());
  fo = true;
  while (fo) {
    fo = false;
    for (int i=0;i<graph.n();i++){
      int x = dgo[i].second;
      if (graph.Degree(x) == 0) continue;
      if (!graph.IsClique(graph.Neighbors(x))) continue;
      bool ok = true;
      for (int nx : graph.Neighbors(x)) {
        if (graph.Degree(nx) <= k) {
          ok = false;
          break;
        }
      }
      if (ok) {
        fo = true;
        auto nbs = graph.Neighbors(x);
        for (int y : nbs) {
          graph.RemoveEdge(x, y);
        }
        assert(graph.Degree(x) == 0);
      }
    }
  }
  return SparseGraph(graph.Edges());
}
} // namespace sms