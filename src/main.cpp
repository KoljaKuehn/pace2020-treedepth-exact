#include <iostream>
#include <exception>
#include <string>

#include "io.hpp"
#include "solve.hpp"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <k>\n"
              << "Reads a graph in PACE tdp format from stdin.\n"
              << "Exits 0 if td(G) <= k, 1 otherwise.\n";
    return 2;
  }
  int k;
  try {
    k = std::stoi(argv[1]);
  } catch (const std::exception&) {
    std::cerr << "Error: k must be an integer, got: " << argv[1] << "\n";
    return 2;
  }
  sms::Io io;
  sms::SparseGraph graph = io.ReadGraph(std::cin);
  const bool result = SolveDecision(graph, k);
  std::cout << (result ? "true" : "false") << "\n";
  return result ? 0 : 1;
}