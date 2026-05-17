#pragma once

#include "graph.hpp"

// Returns true iff the treedepth of graph is at most k.
// Precondition: k >= 1.
bool SolveDecision(const sms::SparseGraph& graph, int k);