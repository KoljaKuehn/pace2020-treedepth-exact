#pragma once

#include <vector>
#include <set>
#include <queue>

#include "utils.hpp"
#include "staticset.hpp"
#include "bitset.hpp"

namespace sms {

typedef std::pair<int, int> Edge;

class SparseGraph;
class FGraph;

class SparseGraph {
 public:
  explicit SparseGraph(int n);
  explicit SparseGraph(std::vector<Edge> edges);
  explicit SparseGraph(const FGraph& graph);

  int n() const;
  int m() const;
  bool HasEdge(int v, int u) const;
  bool HasEdge(Edge e) const;

  void AddEdge(int v, int u);
  void AddEdge(Edge e);

  void RemoveEdge(int v, int u);

  const std::vector<int>& Neighbors(int v) const;

  int Degree(int v) const;

  std::vector<Edge> Edges() const;

  int MapBack(int v) const;

  std::vector<std::vector<int> > Components(const std::vector<int>& separator) const;
  std::vector<int> FindComponentAndMark(int v, std::vector<char>& block) const;

  StaticSet<int> VertexMap() const;

  bool IsClique(const std::vector<int>& vs) const;
  void ShuffleAdjList(std::mt19937& gen);

  int Mincut(int a, int b) const;

 private:
  int n_,m_;
  StaticSet<int> vertex_map_;
  std::vector<std::vector<int> > adj_list_;
  void Dfs(int v, std::vector<char>& blocked, std::vector<int>& component) const;
};

class FGraph {
 public:
  FGraph(int n);
  FGraph(std::vector<Edge> edges);
  FGraph(const SparseGraph& graph);
  int n() const;
  int m() const;
  bool HasEdge(int v, int u) const;
  bool HasEdge(Edge e) const;
  void AddEdge(int v, int u);
  void AddEdge(Edge e);
  const std::vector<int>& Neighbors(int v) const;
  std::vector<FBitset> CompNeighsBit(const FBitset& block) const;
  void Dfs2Bit(FBitset& vis, FBitset& ne) const;
  std::vector<FBitset> SmallMinsepsHeuristic(int sz) const;
  void Dfs(int v, std::vector<char>& block, std::vector<int>& component) const;
  std::vector<int> FindComponentAndMark(int v, std::vector<char>& block) const;
  bool IsConnectedOrIsolated() const;
  std::vector<std::vector<int> > Components(const std::vector<int>& separator) const;
  std::vector<Edge> Edges() const;
  StaticSet<int> VertexMap() const;
  std::vector<Edge> FillEdges(FBitset bs) const;
  int FillSize(FBitset bs) const;
  int Degree(int v) const;
  FBitset Neighbors(const FBitset& vs) const;
  uint64_t Hash2(const FBitset& vert) const;
  std::vector<uint64_t> Labels(const FBitset& vert) const;
  std::vector<uint64_t> RefinedLabels(const FBitset& vert) const;
  std::vector<FBitset> BitComps(FBitset vis) const;
  void ShuffleAdjList(std::mt19937& gen);

  int MaxCompSize(const FBitset& minsep, const FBitset& vert) const;

  bool IsStar(const FBitset& vs) const;
  std::vector<FBitset> StarMinsep(int sz) const;

  std::vector<FBitset> adj_mat2_;
 private:
  int n_, m_;
  StaticSet<int> vertex_map_;
  std::vector<std::vector<int> > adj_list_;
};


inline SparseGraph::SparseGraph(const FGraph& graph) {
  n_ = graph.n();
  m_ = graph.m();
  adj_list_.resize(n_);
  for (int i = 0; i < n_; i++) {
    adj_list_[i] = graph.Neighbors(i);
  }
  vertex_map_ = graph.VertexMap();
}


inline FGraph::FGraph(int n) : n_(n), m_(0), adj_list_(n) {
  assert(n_ <= BITS);
  adj_mat2_.resize(n_);
  std::vector<int> identity(n);
  for (int i = 0; i < n; i++) {
    identity[i] = i;
    adj_mat2_[i].SetTrue(i);
  }
  vertex_map_.Init(identity);
}

inline FGraph::FGraph(std::vector<Edge> edges) : vertex_map_(edges) {
  n_ = vertex_map_.Size();
  assert(n_ <= BITS);
  m_ = 0;
  adj_list_.resize(n_);
  adj_mat2_.resize(n_);
  for (int i = 0; i < n_; i++) {
    adj_mat2_[i].SetTrue(i);
  }
  for (auto edge : edges) {
    AddEdge(vertex_map_.Rank(edge.first), vertex_map_.Rank(edge.second));
  }
}

inline FGraph::FGraph(const SparseGraph& graph) {
  n_ = graph.n();
  assert(n_ <= BITS);
  m_ = 0;
  adj_list_.resize(n_);
  adj_mat2_.resize(n_);
  for (int i = 0; i < n_; i++) {
    adj_mat2_[i].SetTrue(i);
  }
  for (auto edge : graph.Edges()) {
    assert(0 <= edge.first && edge.first < edge.second && edge.second < n_);
    AddEdge(edge.first, edge.second);
  }
  vertex_map_ = graph.VertexMap();
}

inline int FGraph::n() const { return n_; }
inline int FGraph::m() const { return m_; }

inline bool FGraph::HasEdge(int v, int u) const { return adj_mat2_[v].Get(u); }
inline bool FGraph::HasEdge(Edge e)       const { return HasEdge(e.first, e.second); }

inline void FGraph::AddEdge(int v, int u) {
  if (HasEdge(v, u)) return;
  assert(v != u);
  m_++;
  adj_list_[v].push_back(u);
  adj_list_[u].push_back(v);
  adj_mat2_[v].SetTrue(u);
  adj_mat2_[u].SetTrue(v);
}
inline void FGraph::AddEdge(Edge e) { AddEdge(e.first, e.second); }

inline const std::vector<int>& FGraph::Neighbors(int v) const { return adj_list_[v]; }

inline std::vector<FBitset> FGraph::CompNeighsBit(const FBitset& block) const {
  FBitset vis = ~block;
  std::vector<FBitset> ret;
  FBitset ne, sep;
  for (int i = 0; i < n_; i++) {
    if (vis.Get(i)) {
      ne = adj_mat2_[i];
      Dfs2Bit(vis, ne);
      sep.SetAnd(block, ne);
      if (sep.Popcount() > 0) ret.push_back(sep);
    }
  }
  return ret;
}

inline void FGraph::Dfs2Bit(FBitset& vis, FBitset& ne) const {
  bool fo = true;
  while (fo) {
    fo = false;
    uint64_t gv = vis.data_ & ne.data_;
    vis.data_ &= ~gv;
    if (gv) fo = true;
    while (gv) {
      int x = __builtin_ctzll(gv);
      gv &= ~-gv;
      ne |= adj_mat2_[x];
    }
  }
}

inline std::vector<FBitset> FGraph::SmallMinsepsHeuristic(int sz) const {
  assert(IsConnectedOrIsolated());
  std::vector<FBitset> minseps;
  FBitsetSet ff(n_, 2);
  for (int i = 0; i < n_; i++) {
    if (Neighbors(i).empty()) continue;
    for (const FBitset& nbs : CompNeighsBit(adj_mat2_[i])) {
      if (nbs.Popcount() <= sz && ff.Insert(nbs)) {
        minseps.push_back(nbs);
      }
    }
  }
  FBitset vis, sep, ne, mask;
  for (int i = 0; i < n_; i++) {
    if (!Neighbors(i).empty()) mask.SetTrue(i);
  }
  for (int i = 0; i < (int)minseps.size(); i++) {
    const FBitset tsep = minseps[i];
    for (int j : tsep) {
      FBitset block = minseps[i];
      block |= adj_mat2_[j];
      vis.SetNegAnd(block, mask);
      while (vis.data_ > 0) {
        int k = __builtin_ctzll(vis.data_);
        sep = block;
        ne = adj_mat2_[k];
        Dfs2Bit(vis, ne);
        sep.SetAnd(ne, block);
        if (sep.Popcount() <= sz && ff.Insert(sep)) {
          minseps.push_back(sep);
        }
      }
    }
  }
  for (int i = 0; i < (int)minseps.size(); i++) {
    assert(minseps[i].Popcount() <= sz);
  }
  return minseps;
}

inline void FGraph::Dfs(int v, std::vector<char>& block, std::vector<int>& component) const {
  block[v] = true;
  component.push_back(v);
  for (int nv : adj_list_[v]) {
    if (!block[nv]) Dfs(nv, block, component);
  }
}

inline std::vector<int> FGraph::FindComponentAndMark(int v, std::vector<char>& block) const {
  std::vector<int> component;
  Dfs(v, block, component);
  return component;
}

inline bool FGraph::IsConnectedOrIsolated() const {
  auto cs = Components({});
  int f = 0;
  for (const auto& c : cs) {
    if ((int)c.size() > 1) f++;
  }
  return f <= 1;
}

inline std::vector<std::vector<int> > FGraph::Components(const std::vector<int>& separator) const {
  std::vector<char> blocked(n_);
  for (int v : separator) blocked[v] = true;
  std::vector<std::vector<int> > components;
  for (int i = 0; i < n_; i++) {
    if (!blocked[i]) components.push_back(FindComponentAndMark(i, blocked));
  }
  return components;
}

inline std::vector<Edge> FGraph::Edges() const {
  std::vector<Edge> ret;
  for (int i = 0; i < n_; i++) {
    for (int a : adj_list_[i]) {
      if (a > i) ret.push_back({i, a});
    }
  }
  return ret;
}

inline StaticSet<int> FGraph::VertexMap() const { return vertex_map_; }

inline std::vector<Edge> FGraph::FillEdges(FBitset bs) const {
  std::vector<Edge> ret;
  while (bs.data_) {
    int v = __builtin_ctzll(bs.data_);
    bs.data_ &= ~-bs.data_;
    uint64_t td = bs.data_ & ~adj_mat2_[v].data_;
    while (td) {
      int u = __builtin_ctzll(td);
      td &= ~-td;
      ret.push_back({v, u});
    }
  }
  return ret;
}

inline int FGraph::FillSize(FBitset bs) const {
  int ans = 0;
  while (bs.data_) {
    int v = __builtin_ctzll(bs.data_);
    bs.data_ &= ~-bs.data_;
    ans += __builtin_popcountll(bs.data_ & ~adj_mat2_[v].data_);
  }
  return ans;
}

inline int FGraph::Degree(int v) const { return adj_list_[v].size(); }

inline FBitset FGraph::Neighbors(const FBitset& vs) const {
  FBitset nbs;
  for (int v : vs) nbs |= adj_mat2_[v];
  nbs.TurnOff(vs);
  return nbs;
}

inline std::vector<uint64_t> FGraph::RefinedLabels(const FBitset& vert) const {
  std::vector<uint64_t> vh(n_);
  std::vector<FBitset> reach(n_);
  int tn = vert.Popcount();
  for (int v : vert) vh[v] = 1;
  int iters = 0;
  while (1) {
    for (int v : vert) { reach[v].Clear(); reach[v].SetTrue(v); }
    for (int r = 0; r < tn; r++) {
      std::vector<uint64_t> vh_new(n_);
      bool fo = false;
      for (int v : vert) {
        if (reach[v] != vert) {
          fo = true;
          reach[v] |= Neighbors(reach[v]);
          reach[v] &= vert;
          uint64_t nhv = 1;
          for (int u : adj_list_[v]) {
            if (vert.Get(u)) { nhv *= (vh[u] + 1ull); nhv %= 1000000007ull; }
          }
          PolyHash p; p.Add(nhv); p.Add(vh[v]);
          vh_new[v] = p.Value();
        } else {
          vh_new[v] = vh[v];
        }
      }
      vh = vh_new;
      if (!fo) break;
    }
    bool bad = false;
    uint64_t min_bad = 0;
    std::set<uint64_t> hl;
    for (int v : vert) {
      if (hl.count(vh[v])) {
        if (!bad) { bad = true; min_bad = vh[v]; }
        else { min_bad = std::min(min_bad, vh[v]); }
      } else {
        hl.insert(vh[v]);
      }
    }
    if (!bad) return vh;
    iters++;
    assert(iters <= 2*tn + 10);
    for (int v : vert) {
      if (vh[v] == min_bad) {
        PolyHash p; p.Add(vh[v]); p.Add(vh[v]); vh[v] = p.Value(); break;
      }
    }
  }
}

inline std::vector<uint64_t> FGraph::Labels(const FBitset& vert) const {
  std::vector<uint64_t> vh(n_);
  std::vector<FBitset> reach(n_);
  int tn = vert.Popcount();
  for (int v : vert) { reach[v].Clear(); reach[v].SetTrue(v); vh[v] = 1; }
  for (int r = 0; r < tn; r++) {
    std::vector<uint64_t> vh_new(n_);
    bool fo = false;
    for (int v : vert) {
      if (reach[v] != vert) {
        fo = true;
        reach[v] |= Neighbors(reach[v]);
        reach[v] &= vert;
        uint64_t nhv = 1;
        for (int u : adj_list_[v]) {
          if (vert.Get(u)) { nhv *= (vh[u] + 1ull); nhv %= 1000000007ull; }
        }
        nhv += 1ll;
        nhv *= (vh[v] + 2ull);
        nhv %= 1000000007ull;
        vh_new[v] = nhv;
      } else {
        vh_new[v] = vh[v];
      }
    }
    vh = vh_new;
    if (!fo) break;
  }
  return vh;
}

inline uint64_t FGraph::Hash2(const FBitset& vert) const {
  auto vh = Labels(vert);
  std::sort(vh.begin(), vh.end());
  PolyHash p;
  for (uint64_t v : vh) { if (v > 0) p.Add(v); }
  return p.Value();
}

inline std::vector<FBitset> FGraph::BitComps(FBitset vis) const {
  FBitset ne;
  std::vector<FBitset> ret;
  bool fo = false;
  while (1) {
    if (!fo) {
      if (vis.data_) {
        int x = __builtin_ctzll(vis.data_);
        ne.SetTrue(x);
        ret.push_back(FBitset());
        fo = true;
      }
      if (!fo) return ret;
    }
    fo = false;
    uint64_t gv = vis.data_ & ne.data_;
    while (gv) {
      fo = true;
      vis.data_ &= ~(gv & -gv);
      int x = __builtin_ctzll(gv);
      ne |= adj_mat2_[x];
      ret.back().SetTrue(x);
      gv &= ~-gv;
    }
  }
}

inline void FGraph::ShuffleAdjList(std::mt19937& gen) {
  for (int i = 0; i < n_; i++) {
    std::shuffle(adj_list_[i].begin(), adj_list_[i].end(), gen);
  }
}

inline bool FGraph::IsStar(const FBitset& vs) const {
  bool fb = false;
  for (int v : vs) {
    if (vs.IntersectionPopcount(adj_mat2_[v]) > 2) {
      if (fb) return false;
      else fb = true;
    }
  }
  return true;
}

inline std::vector<FBitset> FGraph::StarMinsep(int sz) const {
  assert(IsConnectedOrIsolated());
  FBitset mask;
  for (int i = 0; i < n_; i++) {
    if (!Neighbors(i).empty()) mask.SetTrue(i);
  }
  for (int i = 0; i < n_; i++) {
    if (Degree(i) == 0) continue;
    FBitset rch = mask;
    for (int v : adj_mat2_[i]) rch.TurnOff(adj_mat2_[v]);
    bool ok1 = true;
    for (const auto& comp : BitComps(rch)) {
      if (!IsStar(comp)) { ok1 = false; break; }
    }
    if (!ok1) continue;
    std::vector<FBitset> minseps;
    FBitsetSet ff(2, 2);
    for (const FBitset& nbs : CompNeighsBit(adj_mat2_[i])) {
      if (ff.Insert(nbs)) minseps.push_back(nbs);
    }
    for (int it = 0; it < (int)minseps.size(); it++) {
      FBitset tsep = minseps[it];
      FBitset vv = mask;
      vv.TurnOff(tsep);
      bool ok = false;
      for (const auto& comp : BitComps(vv)) {
        if (comp.Get(i) && IsStar(comp) && Neighbors(comp) == tsep) { ok = true; break; }
      }
      if (!ok) continue;
      if (tsep.Popcount() <= sz) {
        assert(ok);
        for (const auto& comp : BitComps(vv)) {
          if (!IsStar(comp)) { ok = false; break; }
        }
        if (ok) return {tsep};
      }
      for (int j : tsep) {
        if (!adj_mat2_[i].Get(j)) continue;
        FBitset block = tsep;
        block |= adj_mat2_[j];
        FBitset vis;
        vis.SetNegAnd(block, mask);
        while (vis.data_ > 0) {
          int k = __builtin_ctzll(vis.data_);
          FBitset sep = block;
          FBitset ne = adj_mat2_[k];
          Dfs2Bit(vis, ne);
          sep.SetAnd(ne, block);
          if (ff.Insert(sep)) minseps.push_back(sep);
        }
      }
    }
  }
  return {};
}

inline int FGraph::MaxCompSize(const FBitset& minsep, const FBitset& vert) const {
  FBitset vis = vert;
  vis.TurnOff(minsep);
  int ret = 0;
  while (vis.data_ > 0) {
    int k = __builtin_ctzll(vis.data_);
    FBitset ne = adj_mat2_[k];
    Dfs2Bit(vis, ne);
    ne.data_ &= ~vis.data_;
    ret = std::max(ret, ne.Popcount());
    if (ret > vis.Popcount()) return ret;
  }
  return ret;
}

inline void SepRec(const FGraph& graph, int a, int b, FBitset neA, FBitset neB, FBitset F,
                   std::vector<FBitset>& minseps, int sz, int n) {
  assert(F.Popcount() <= sz);
  FBitset inter = neA;
  inter &= neB;
  assert(inter.Subsumes(F));
  if (inter == F) { minseps.push_back(F); return; }
  if (F.Popcount() == sz) return;

  int szthr = std::min(neB.Popcount() - inter.Popcount(), (n - F.Popcount()) / 2);
  int a_size = neA.Popcount() - inter.Popcount();
  if (a_size > szthr) return;
  if (neA.Popcount() - sz > szthr) return;
  inter.TurnOff(F);
  for (int v : inter) {
    if (graph.HasEdge(b, v)) {
      F.SetTrue(v);
      SepRec(graph, a, b, neA, neB, F, minseps, sz, n);
      return;
    }
  }

  if (a_size + 3*(inter.Popcount() - (sz - F.Popcount())) > szthr) {
    std::vector<std::vector<int>> paths;
    for (int v : inter) paths.push_back({v});
    FBitset space = neB;
    space.TurnOff(F);
    space.TurnOff(inter);
    int d = 0;
    int cantake = (int)paths.size() - (sz - F.Popcount());
    for (int len = 1; !paths.empty() && cantake > 0; len++) {
      for (int i = 0; i < (int)paths.size(); i++) {
        assert((int)paths[i].size() == len);
        if (graph.adj_mat2_[paths[i].back()].Intersects(space)) {
          FBitset lol = graph.adj_mat2_[paths[i].back()] & space;
          int x = lol.First();
          assert(space.Get(x) && x != paths[i].back() && graph.HasEdge(x, paths[i].back()));
          paths[i].push_back(x);
          space.SetFalse(x);
        } else {
          if (cantake) { cantake--; d += len; }
          std::swap(paths[i], paths.back());
          paths.pop_back();
          i--;
        }
      }
    }
    if (a_size + d > szthr) return;
  }
  int x = inter.First();
  F.SetTrue(x);
  SepRec(graph, a, b, neA, neB, F, minseps, sz, n);
  F.SetFalse(x);

  FBitset vis = neB;
  vis.TurnOff(neA);
  vis.TurnOff(graph.adj_mat2_[x]);
  neB = graph.adj_mat2_[b];
  graph.Dfs2Bit(vis, neB);
  if (!neB.Subsumes(F)) return;

  vis.FillTrue();
  vis.TurnOff(neB);
  graph.Dfs2Bit(vis, neA);
  SepRec(graph, a, b, neA, neB, F, minseps, sz, n);
}

inline std::vector<FBitset> NibbleSmallMinseps(FGraph graph, int sz) {
  assert(graph.IsConnectedOrIsolated());
  int mfi = graph.n() * graph.n();
  int mfv = graph.n();
  FBitset vert;
  for (int i = 0; i < graph.n(); i++) {
    if (graph.Degree(i) > 0) {
      vert.SetTrue(i);
      int fi = graph.FillSize(graph.adj_mat2_[i]);
      if (fi < mfi) { mfi = fi; mfv = i; }
    }
  }
  if (vert.Popcount() <= 2) return {};
  assert(mfv < graph.n() && mfi < graph.n() * graph.n());
  std::vector<FBitset> minseps;
  for (int a : graph.Neighbors(mfv)) {
    for (int b : graph.Neighbors(mfv)) {
      if (a == b || graph.HasEdge(a, b)) continue;
      FBitset F = graph.adj_mat2_[a];
      F &= graph.adj_mat2_[b];
      if (F.Popcount() <= sz) {
        FBitset vis = vert;
        vis.TurnOff(graph.adj_mat2_[a]);
        FBitset neB = graph.adj_mat2_[b];
        graph.Dfs2Bit(vis, neB);
        SepRec(graph, a, b, graph.adj_mat2_[a], neB, F, minseps, sz, vert.Popcount());
      }
    }
  }
  vert.TurnOff(graph.adj_mat2_[mfv]);
  for (auto comp : graph.BitComps(vert)) {
    FBitset nbs = graph.Neighbors(comp);
    if (nbs.Popcount() <= sz) minseps.push_back(nbs);
    FGraph ngraph(graph.n());
    comp |= nbs;
    for (auto e : graph.Edges()) {
      if (comp.Get(e.first) && comp.Get(e.second)) ngraph.AddEdge(e);
    }
    for (auto e : graph.FillEdges(nbs)) ngraph.AddEdge(e);
    auto rms = NibbleSmallMinseps(ngraph, sz);
    for (const auto& sep : rms) minseps.push_back(sep);
  }
  utils::SortAndDedup(minseps);
  return minseps;
}

} // namespace sms
