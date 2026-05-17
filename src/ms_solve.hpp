#pragma once

#include <map>
#include <vector>
#include <cstdint>

#include "graph.hpp"
#include "bitset.hpp"

namespace sms {

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

} // namespace sms