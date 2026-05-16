#pragma once

#include <vector>
#include <algorithm>
#include <cstdlib>
#include <queue>
#include <random>

#include "bitset.hpp"

namespace sms {
namespace utils {
template<typename T>
void SortAndDedup(std::vector<T>& vec);

template<typename T>
T GetRand(T a, T b, std::mt19937& gen);
} // namespace utils

class PolyHash {
 private:
  uint64_t val_;
 public:
  PolyHash() {
    val_ = 0;
  }
  void Add(uint64_t n) {
    n %= 1000000007;
    val_ *= 65599;
    val_ += n + 59;
    val_ %= 1000000007;
  }
  uint64_t Value() const {
    return val_;
  }
};

namespace utils {
template<typename T>
T GetRand(T a, T b, std::mt19937& gen) {
  return std::uniform_int_distribution<T>(a,b)(gen);
}

template<typename T>
void SortAndDedup(std::vector<T>& vec) {
  std::sort(vec.begin(), vec.end());
  vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}
} // namespace utils
} // namespace sms