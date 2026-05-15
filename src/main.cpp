#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <iomanip>
#include <set>
#include <cassert>
#include <random>
#include <sys/resource.h>

#include "graph.hpp"
#include "io.hpp"
#include "utils.hpp"
#include "mcs.hpp"
#include "staticset.hpp"
#include "bitset.hpp"
#include "chordalsolve.hpp"
#include "best.hpp"
#include "preprocessor.hpp"
#include "ms_solve.hpp"

using namespace sms;

#define F first
#define S second

using std::vector;

std::mt19937 gen(1337);

void SetStackSize(int64_t sz) {
  struct rlimit rl;
  assert(getrlimit(RLIMIT_STACK, &rl) == 0);
  Log::Write(3, "Cur stack size ", rl.rlim_cur);
  if (rl.rlim_cur < sz) {
    rl.rlim_cur = sz;
    Log::Write(3, "Setting stack size ", sz);
    assert(setrlimit(RLIMIT_STACK, &rl) == 0);
  }
}

int HeurComp(const FGraph& graph, int best, double time, const Preprocessor& pp) {
  Timer timer;
  timer.start();
  int it=0;
  std::set<uint64_t> gs;
  int vari = 0;
  int upd_cnt = 0;
  int last_add = 0;
  while (timer.get() < time) {
    double dupls = 0;
    if (it > 0) {
      dupls = (double)(it - (int)gs.size()) / (double)it;
    }
    if (dupls > 0.5 && upd_cnt == graph.n() && it - last_add > 10) {
      vari++;
      last_add = it;
    }
    it++;
    Timer triang_tmr;
    triang_tmr.start();
    FGraph lol_g = graph;
    mcs::LbTriang(lol_g, gen, vari, upd_cnt);
    triang_tmr.stop();
    double est = (double)gs.size() * (double)it / ((double)it - (double)gs.size());
    Log::Write(10, "min tri ", triang_tmr.get(), " ", best, " ", est, " ", lol_g.m(), " ", dupls, " ", vari, " ", upd_cnt);
    upd_cnt = upd_cnt * 2 + 1;
    upd_cnt = std::min(upd_cnt, graph.n());
    if (gs.count(lol_g.Hash())) {
      Log::Write(10, "Same triang ", gs.size(), " ", it, " ", est);
      continue;
    }
    gs.insert(lol_g.Hash());
    {
      Timer td_tmr;
      td_tmr.start();
      ChordalSolve cs(lol_g);
      int td = cs.Solve(best-1, vari, std::min(time - timer.get(), triang_tmr.get() + 0.01));
      if (td < best) {
        best = td;
        Log::Write(3, "Treedepth: ", best);
        auto resu = cs.Get(best);
        resu = pp.Reconstruct(resu);
        resu = ColToPar(pp.org_graph, resu);
        int got = best::SetBest(resu, true);
        assert(got <= td);
        best = got;
        Log::Write(3, "Got ", got);
      }
    }
  }
  return best;
}

int DoSolve2(const SparseGraph& graph, int best, const Preprocessor& pp) {
  assert(graph.n() <= BITS);
  FGraph ppg(graph);
  Log::Write(3, "Solve2 n:", ppg.n(), " m:", ppg.m());
  {
    MSSolve mss(ppg);
    mss.incorrect_msenum_ = true;
    int ans = mss.Solve(best-1, true);
    if (ans < best) {
      best = ans;
      Log::Write(3, "Heur ans ", ans);
      auto sol = mss.Get(ans);
      sol = pp.Reconstruct(sol);
      sol = ColToPar(pp.org_graph, sol);
      int got = best::SetBest(sol, true);
      assert(got <= ans);
      Log::Write(3, "Ans valid ", got, " ", ans);
      best = got;
      Log::Write(3, "Re preprocess");
      return best;
    }
  }
  MSSolve mss2(ppg);
  int ans2 = mss2.Solve(best-1, false);
  if (ans2 < best) {
    best = ans2;
    Log::Write(3, "Exact ans ", ans2);
    auto sol = mss2.Get(ans2);
    sol = pp.Reconstruct(sol);
    sol = ColToPar(pp.org_graph, sol);
    int got = best::SetBest(sol, true);
    assert(got == ans2);
    Log::Write(3, "Ans valid ", ans2);
  }
  return -1;
}

void DoSolve1(const SparseGraph& graph, int best, const Preprocessor& pp) {
  assert(graph.n() <= BITS);
  const FGraph ppg(graph);
  Log::Write(3, "Dosolve1 n:", ppg.n(), " m:", ppg.m());

  double pp_time = 40;
  if      (ppg.n() <= 50)  pp_time = 1;
  else if (ppg.n() <= 75)  pp_time = 5;
  else if (ppg.n() <= 100) pp_time = 20;
  else if (ppg.n() <= 150) pp_time = 30;
  else if (ppg.n() <= 200) pp_time = 40;
  else if (ppg.n() <= 250) pp_time = 50;
  else                     pp_time = 60;

  best = HeurComp(ppg, best, pp_time, pp);

  while (true) {
    Preprocessor pp2 = pp;
    SparseGraph pp_graph = pp2.TamakiRules(SparseGraph(ppg), best-1);
    assert(pp_graph.n() <= BITS);
    int nbest = DoSolve2(pp_graph, best, pp2);
    if (nbest == -1) return;
    assert(nbest >= 0 && nbest < best);
    best = nbest;
    Log::Write(3, "Re solve ", best);
  }
}

int main() {
  SetStackSize(8ll * 1024 * 1024);
  Log::SetLogLevel(3);
  Io io;
  SparseGraph graph = io.ReadGraph(std::cin);

  Log::Write(3, "Input n:", graph.n(), " m:", graph.m());
  assert(graph.n() <= BITS);
  best::InitBest(graph);
  assert(graph.IsConnected());
  int best = graph.n();

  Preprocessor pp;
  SparseGraph pp_graph = pp.Preprocess(graph);

  assert(pp_graph.n() <= BITS);
  DoSolve1(pp_graph, best, pp);
  best::PrintBest();
}