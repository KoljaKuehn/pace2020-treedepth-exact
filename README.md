# pace2020-treedepth-exact (graph_generation fork)

This is a fork of `SMS`, the PACE 2020 exact treedepth submission by
Tuukka Korhonen (University of Helsinki). The original solver branches
on small minimal separators; see [https://arxiv.org/abs/2006.07302](https://arxiv.org/abs/2006.07302).
Upstream code: [https://doi.org/10.5281/zenodo.3872898](https://doi.org/10.5281/zenodo.3872898).

This `graph_generation` branch trims the solver down to a single
purpose: answering treedepth decision queries for many small graphs as
fast as possible. It is used as an embedded library by another project,
not as a standalone PACE entry.


## What is different from upstream

- **Decision-only API.** The library exposes
  `bool SolveDecision(const sms::SparseGraph& graph, int k)` which
  returns whether `td(graph) <= k`. There is no witness reconstruction
  and no iterative descent from an upper bound.
- **No heuristic presolve.** The ChordalSolve / MCS-based upper-bound
  search and the `incorrect_msenum_` fast path are removed. Every call
  goes straight into the exact minimal-separator branch-and-bound at
  the requested `k`.
- **Graphs limited to 64 vertices.** All `FBitset` / `FGraph` chunk
  templates collapse to a single `uint64_t`. Inputs with more than 64
  vertices are rejected by `assert`.
- **Thread-safe.** Globals that the solver mutates were converted to
  `thread_local`, so multiple threads can call `SolveDecision`
  concurrently on independent inputs.
- **Disconnected graphs handled.** `SolveDecision` decomposes its input
  into connected components and solves each independently.
- **Quiet by default.** All logging, timer, and output infrastructure
  has been removed; the library is silent.
- **Built as a static library.** `libtd_exact.a` is produced from the
  `add_library(td_exact STATIC ...)` target in `src/CMakeLists.txt`. A
  small `td_exact_standalone` test binary that reads a PACE `.tdp`
  graph from stdin is built only when `src/` is configured as a
  top-level CMake project.


## Building

```
cmake -S src -B build
cmake --build build
```

This produces `build/libtd_exact.a` and, when configured standalone,
`build/td_exact_standalone`. The standalone takes `k` as its sole
argument and exits 0 if `td(G) <= k`, 1 otherwise.