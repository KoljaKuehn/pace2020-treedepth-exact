#pragma once

#include <map>
#include <random>

#include "graph.hpp"
#include "bitset.hpp"

#define F first
#define S second

namespace sms {
using std::max;
using std::min;
using std::vector;

struct Piece {
  Piece() { lb = -1; ub = (int)1e9; }
  int lb, ub;
};

struct MsPiece {
  std::vector<FBitset> minseps;
};

class MSSolve {
 public:
  MSSolve(const FGraph& graph);
  bool Solve(int k);

  std::map<uint64_t, std::vector<FBitset>> isom_map_;

 private:
  int PieceId(const FBitset& piece, bool insert, bool expect);
  int MsPieceId(const FBitset& piece, bool insert, bool expect);
  FGraph graph_;
  std::vector<Piece> pcs_;
  std::vector<MsPiece> ms_pcs_;
  FBitsetMap bs_cac_;
  FBitsetMap ms_bs_cac_;
  FLBSieve lb_sieve_;
  bool Go(FBitset vert, int k, const std::vector<Edge>& parent_edges,
          const std::vector<FBitset>& parent_minseps, int parent_n, bool can_induce_seps);
  bool Isom(const FBitset& v1, const FBitset& v2) const;
};


namespace {
int PathLb(int length) {
  if (length <= 0) return 0;
  uint32_t x = length;
  return 32 - __builtin_clz(x);
}
int CycleLb(int length) {
  if (length <= 0) return PathLb(length);
  uint32_t x = length;
  return 33 - __builtin_clz(x-1);
}
thread_local std::vector<int> path_cycle_found;
void pclbdfs(const SparseGraph& graph, int x, std::vector<int>& d, int& lb, std::vector<int>& pt) {
  assert(d[x] > 0);
  if (PathLb(d[x]) > lb) {
    lb = PathLb(d[x]);
    path_cycle_found.clear();
    int p = x;
    while (d[p] > 1) { path_cycle_found.push_back(p); p = pt[p]; }
    path_cycle_found.push_back(p);
    assert((int)path_cycle_found.size() == d[x]);
  }
  for (int nx : graph.Neighbors(x)) {
    if (d[nx]) {
      if (d[nx] < d[x]) {
        if (CycleLb(d[x] - d[nx] + 1) > lb) {
          lb = CycleLb(d[x] - d[nx] + 1);
          path_cycle_found.clear();
          int p = x;
          while (p != nx) { path_cycle_found.push_back(p); p = pt[p]; }
          path_cycle_found.push_back(p);
          assert((int)path_cycle_found.size() == d[x] - d[nx] + 1);
        }
      }
    } else {
      d[nx] = d[x]+1;
      pt[nx] = x;
      pclbdfs(graph, nx, d, lb, pt);
    }
  }
}

thread_local std::mt19937 gener(1337);

int PathCycleLb(SparseGraph graph, int rounds, int goal) {
  int lb = goal - 1;
  std::vector<int> d(graph.n()), pt(graph.n());
  for (int it = 0; it < rounds; it++) {
    graph.ShuffleAdjList(gener);
    for (int i = 0; i < graph.n(); i++) {
      if (graph.Degree(i) > 0) {
        std::fill(d.begin(), d.end(), 0);
        d[i] = 1;
        pclbdfs(graph, i, d, lb, pt);
        if (lb >= goal) return lb;
      }
    }
  }
  return 0;
}

int MMDP(FGraph graph) {
  std::vector<std::vector<int>> q(graph.n());
  std::vector<int> dg(graph.n());
  FBitset vert;
  int vs = 0;
  for (int i = 0; i < graph.n(); i++) {
    dg[i] = graph.Degree(i);
    if (dg[i] > 0) { q[dg[i]].push_back(i); vs++; vert.SetTrue(i); }
  }
  int mt = 0, t = 0;
  for (int it = 0; it < vs; it++) {
    int x = -1;
    while (x == -1) {
      if (q[t].empty()) { t++; }
      else {
        x = q[t].back(); q[t].pop_back();
        if (dg[x] != t) x = -1;
      }
    }
    mt = std::max(mt, t);
    assert(x>=0 && x<graph.n() && dg[x]==t && t>=0 && vert.Get(x));
    assert(dg[x]+1 == (graph.adj_mat2_[x] & vert).Popcount());
    int cto = -1, cns = graph.n();
    for (int nx : graph.Neighbors(x)) {
      if (dg[nx] >= 0) {
        assert(dg[nx] >= 1 && vert.Get(nx));
        FBitset cm = vert; cm &= graph.adj_mat2_[x]; cm &= graph.adj_mat2_[nx];
        assert(cm.Get(x) && cm.Get(nx));
        if (cm.Popcount() < cns) { cns = cm.Popcount(); cto = nx; }
      }
    }
    if (cto == -1) { assert(vert.Popcount() == 1); return mt; }
    assert(cto >= 0 && cto < graph.n() && cns >= 2);
    for (int nx : graph.Neighbors(x)) {
      if (dg[nx] >= 0 && nx != cto && !graph.HasEdge(nx, cto)) {
        assert(vert.Get(nx));
        graph.AddEdge(nx, cto);
        dg[nx]++; dg[cto]++;
      }
    }
    for (int nx : graph.Neighbors(x)) {
      if (dg[nx] >= 0) { assert(dg[nx] >= 1 && vert.Get(nx)); dg[nx]--; q[dg[nx]].push_back(nx); }
    }
    dg[x] = -1; vert.SetFalse(x); t--;
  }
  return mt;
}
} // namespace


inline MSSolve::MSSolve(const FGraph& graph) : graph_(graph) {
  assert(!bs_cac_.Inited());
  bs_cac_    = FBitsetMap(2, 1.5);
  ms_bs_cac_ = FBitsetMap(2, 1.5);
  assert(bs_cac_.Inited() && ms_bs_cac_.Inited());
  pcs_.push_back({});
  ms_pcs_.push_back({});
  lb_sieve_ = FLBSieve(graph.n(), graph.n()+1);
}

inline int MSSolve::PieceId(const FBitset& piece, bool insert, bool expect) {
  assert(pcs_.size() >= 1);
  if (insert) {
    int id = bs_cac_.Insert(piece, (int)pcs_.size()).first;
    assert(id > 0);
    if (id == (int)pcs_.size()) { pcs_.push_back({}); assert(!expect); return id; }
    else { assert(id < (int)pcs_.size()); return id; }
  } else {
    int id = bs_cac_.Get(piece, expect);
    assert(id >= 0);
    if (expect) assert(id > 0);
    assert(id < (int)pcs_.size());
    return id;
  }
}

inline int MSSolve::MsPieceId(const FBitset& piece, bool insert, bool expect) {
  assert(ms_pcs_.size() >= 1);
  if (insert) {
    int id = ms_bs_cac_.Insert(piece, (int)ms_pcs_.size()).first;
    assert(id > 0);
    if (id == (int)ms_pcs_.size()) { ms_pcs_.push_back({}); assert(!expect); return id; }
    else { assert(id < (int)ms_pcs_.size()); return id; }
  } else {
    int id = ms_bs_cac_.Get(piece, expect);
    assert(id >= 0);
    if (expect) assert(id > 0);
    assert(id < (int)ms_pcs_.size());
    return id;
  }
}

inline bool MSSolve::Isom(const FBitset& v1, const FBitset& v2) const {
  std::vector<uint64_t> l1, l2;
  l1 = graph_.RefinedLabels(v1);
  l2 = graph_.RefinedLabels(v2);
  for (int i = 0; i < graph_.n(); i++) {
    if (!v1.Get(i)) l1[i] = 0;
    if (!v2.Get(i)) l2[i] = 0;
  }
  std::vector<std::pair<int, int>> lp1(graph_.n()), lp2(graph_.n());
  for (int i = 0; i < graph_.n(); i++) {
    lp1[i] = {l1[i], i}; lp2[i] = {l2[i], i};
  }
  std::sort(lp1.begin(), lp1.end());
  std::sort(lp2.begin(), lp2.end());
  std::vector<int> m1(graph_.n()), m2(graph_.n());
  for (int i = 0; i < graph_.n(); i++) {
    if (lp1[i].F != lp2[i].F) return false;
    m1[lp1[i].S] = lp2[i].S;
    m2[lp2[i].S] = lp1[i].S;
  }
  for (int i = 0; i < graph_.n(); i++) {
    if (v1.Get(i)) {
      for (int ni : graph_.Neighbors(i)) {
        if (v1.Get(ni)) {
          if (!graph_.HasEdge(m1[i], m1[ni]) || !v2.Get(m1[i]) || !v2.Get(m1[ni])) {
            return false;
          }
        }
      }
    }
  }
  for (int i = 0; i < graph_.n(); i++) {
    if (v2.Get(i)) {
      for (int ni : graph_.Neighbors(i)) {
        if (v2.Get(ni)) {
          if (!graph_.HasEdge(m2[i], m2[ni]) || !v1.Get(m2[i]) || !v1.Get(m2[ni])) {
            return false;
          }
        }
      }
    }
  }
  assert(v1.Popcount() == v2.Popcount());
  return true;
}

inline bool MSSolve::Go(FBitset vert, int k, const vector<Edge>& parent_edges,
                         const vector<FBitset>& parent_minseps, int parent_n, bool can_induce_seps) {
  int pc_id = PieceId(vert, true, false);
  int n = vert.Popcount();
  if (n == 1) pcs_[pc_id].lb = 1;
  else        pcs_[pc_id].lb = max(pcs_[pc_id].lb, 2);
  if (pcs_[pc_id].lb > k) return false;
  pcs_[pc_id].ub = min(pcs_[pc_id].ub, n);
  if (pcs_[pc_id].ub <= k) return true;

  FGraph t_graph(graph_.n());
  std::vector<Edge> tg_edges;
  for (auto e : parent_edges) {
    if (vert.Get(e.F) && vert.Get(e.S)) { tg_edges.push_back(e); t_graph.AddEdge(e); }
  }

  if ((int64_t)t_graph.m() == (int64_t)n*(n-1)/2) {
    pcs_[pc_id].ub = n; pcs_[pc_id].lb = n; return false;
  } else {
    assert((int64_t)t_graph.m() < (int64_t)n*(n-1)/2);
    pcs_[pc_id].ub = min(pcs_[pc_id].ub, n - 1);
    if (pcs_[pc_id].ub <= k) return true;
  }

  int mmdp = MMDP(t_graph);
  pcs_[pc_id].lb = max(pcs_[pc_id].lb, 1 + mmdp);
  if (pcs_[pc_id].lb > k) return false;
  if (CycleLb(n) > k) {
    int lb = PathCycleLb(SparseGraph(t_graph), (CycleLb(n)-k)*CycleLb(n), k+1);
    pcs_[pc_id].lb = max(pcs_[pc_id].lb, lb);
    if (pcs_[pc_id].lb > k) {
      FBitset sg;
      for (int v : path_cycle_found) sg.SetTrue(v);
      lb_sieve_.Insert(sg, pcs_[pc_id].lb);
      return false;
    }
  }
  assert(n >= 4 && t_graph.m() >= 3);

  FBitset cover;
  for (int v : vert) {
    if (t_graph.Degree(v) < k) {
      cover = t_graph.adj_mat2_[v]; cover.SetFalse(v);
      bool isvc = true;
      for (auto e : tg_edges) {
        if (!cover.Get(e.F) && !cover.Get(e.S)) { isvc = false; break; }
      }
      if (isvc) {
        pcs_[pc_id].ub = t_graph.Degree(v)+1;
        assert(pcs_[pc_id].ub <= k);
        return true;
      }
    }
  }

  std::vector<FBitset> star_minseps = t_graph.StarMinsep(k-2);
  if (!star_minseps.empty()) {
    assert(star_minseps.size() == 1 && star_minseps[0].Popcount() <= k-2);
    FBitset vv = vert; vv.TurnOff(star_minseps[0]);
    for (const auto& comp : t_graph.BitComps(vv)) assert(t_graph.IsStar(comp));
    Log::Write(5, "FOUND STAR");
    pcs_[pc_id].ub = star_minseps[0].Popcount() + 2;
    return true;
  }

  if (n >= graph_.n() - graph_.n()/3) {
    uint64_t isohash = graph_.Hash2(vert);
    if (isom_map_[isohash].empty()) {
      isom_map_[isohash].push_back(vert);
    } else {
      for (const FBitset& vs : isom_map_[isohash]) {
        if (Isom(vert, vs)) {
          pcs_[pc_id].lb = max(pcs_[pc_id].lb, pcs_[PieceId(vs, false, true)].lb);
          if (pcs_[pc_id].lb > k) {
            lb_sieve_.Insert(vert, pcs_[pc_id].lb);
            return false;
          }
        }
      }
      isom_map_[isohash].push_back(vert);
    }
  }

  std::vector<FBitset> t_minseps;
  {
    std::vector<std::tuple<int, int, FBitset>> tms_sort;
    int enum_sz = k-3;
    bool do1 = false;
    if (n < graph_.n() && can_induce_seps) {
      if (n > parent_n/2) do1 = true;
    }
    if (do1) {
      for (const auto& pms : parent_minseps) {
        int sep_size = vert.IntersectionPopcount(pms);
        if (sep_size == 0 || sep_size > enum_sz) continue;
        FBitset vert2 = vert; vert2.TurnOff(pms);
        FBitset vert4 = pms; vert4 &= vert;
        int fcs = 0, comp_size = 0, sz = n - sep_size;
        for (int i = 0; i < t_graph.n(); i++) {
          if (vert2.Get(i)) {
            FBitset vert3 = t_graph.adj_mat2_[i];
            t_graph.Dfs2Bit(vert2, vert3);
            int nsz = vert2.Popcount();
            comp_size = max(comp_size, sz - nsz);
            sz = nsz;
            if (vert3.Subsumes(vert4)) fcs++;
          }
        }
        if (fcs < 2) continue;
        tms_sort.push_back(std::make_tuple(comp_size, sep_size, vert4));
      }
    } else {
      t_minseps = NibbleSmallMinseps(t_graph, enum_sz);
      tms_sort.resize(t_minseps.size());
      for (int i = 0; i < (int)t_minseps.size(); i++) {
        tms_sort[i] = std::make_tuple(t_graph.MaxCompSize(t_minseps[i], vert), t_minseps[i].Popcount(), t_minseps[i]);
      }
    }
    auto cmp = [&](const std::tuple<int, int, FBitset>& a, const std::tuple<int, int, FBitset>& b) {
      if (std::get<0>(a) != std::get<0>(b)) return std::get<0>(a) < std::get<0>(b);
      if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
      return std::get<2>(a) < std::get<2>(b);
    };
    std::sort(tms_sort.begin(), tms_sort.end(), cmp);
    t_minseps.resize(tms_sort.size());
    for (int i = 0; i < (int)tms_sort.size(); i++) t_minseps[i] = std::get<2>(tms_sort[i]);
    t_minseps.erase(std::unique(t_minseps.begin(), t_minseps.end()), t_minseps.end());
    if (n == graph_.n()) Log::Write(5, "msenum root ", t_minseps.size());
  }

  int max_sep_size = std::min(n - 2, k - 3);
  std::vector<int> degseq;
  for (int i = 0; i < t_graph.n(); i++) {
    if (t_graph.Degree(i) > 0) degseq.push_back(t_graph.Degree(i));
  }
  std::sort(degseq.rbegin(), degseq.rend());
  while (max_sep_size > 0) {
    int max_m = (k - max_sep_size - 1) * (n - max_sep_size - 1);
    int tm = t_graph.m();
    for (int i = 0; i < (int)degseq.size() && i < max_sep_size; i++) tm -= degseq[i];
    if (tm > max_m) max_sep_size--;
    else break;
  }
  if (max_sep_size <= 0) {
    pcs_[pc_id].lb = k+1;
    lb_sieve_.Insert(vert, pcs_[pc_id].lb);
    return false;
  }
  for (const FBitset& ms : t_minseps) {
    if (ms.Popcount() > max_sep_size) continue;
    assert(vert.Subsumes(ms));

    vert.TurnOff(ms);
    if (lb_sieve_.Get(vert, k-ms.Popcount()+1)) { vert |= ms; continue; }
    auto bcs = t_graph.BitComps(vert);
    vert |= ms;

    assert(bcs.size() >= 2);
    auto cmp = [](const FBitset& a, const FBitset& b) { return a.Popcount() > b.Popcount(); };
    std::sort(bcs.begin(), bcs.end(), cmp);
    bool ok = true;
    int tans = 0;
    for (const FBitset& bc : bcs) {
      bool rec_induce = (t_graph.Neighbors(bc) == ms);
      if (!Go(bc, k - ms.Popcount(), tg_edges, t_minseps, n, rec_induce)) { ok = false; break; }
      else tans = max(tans, ms.Popcount() + pcs_[PieceId(bc, false, true)].ub);
    }
    if (ok) {
      assert(tans <= pcs_[pc_id].ub);
      pcs_[pc_id].ub = tans;
      int ms_pc_id = MsPieceId(vert, true, false);
      ms_pcs_[ms_pc_id].minseps.push_back(ms);
      return true;
    }
  }
  pcs_[pc_id].lb = k+1;
  lb_sieve_.Insert(vert, pcs_[pc_id].lb);
  assert(pcs_[pc_id].lb <= pcs_[pc_id].ub);
  return false;
}

inline bool MSSolve::Solve(int k) {
  FBitset vert;
  vert.FillUpTo(graph_.n());
  return Go(vert, k, graph_.Edges(), {}, graph_.n(), false);
}

} // namespace sms