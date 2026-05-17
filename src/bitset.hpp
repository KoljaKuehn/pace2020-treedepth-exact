#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <random>
#include <limits>
#include <cassert>

#define BITS 64

namespace sms {

class FBitset {
 public:
  uint64_t data_;

  void Clear() { data_ = 0; }
  FBitset() : data_(0) {}

  bool operator<(const FBitset& other) const { return data_ < other.data_; }
  bool operator==(const FBitset& other) const { return data_ == other.data_; }
  bool operator!=(const FBitset& other) const { return data_ != other.data_; }

  FBitset operator|(const FBitset& other) const { FBitset r; r.data_ = data_ | other.data_; return r; }
  FBitset operator&(const FBitset& other) const { FBitset r; r.data_ = data_ & other.data_; return r; }
  FBitset operator~()                     const { FBitset r; r.data_ = ~data_;               return r; }

  void SetTrue(size_t i)  { data_ |=  (uint64_t(1) << i); }
  void SetFalse(size_t i) { data_ &= ~(uint64_t(1) << i); }

  void SetTrue(const std::vector<size_t>& v) { for (size_t x : v) SetTrue(x); }
  void SetTrue(const std::vector<int>& v)    { for (int   x : v) SetTrue(x); }
  void SetFalse(const std::vector<int>& v)   { for (int   x : v) SetFalse(x); }

  void FillTrue() { data_ = ~uint64_t(0); }

  void FillUpTo(size_t n) {
    if      (n == 0)    data_ = 0;
    else if (n >= BITS) data_ = ~uint64_t(0);
    else                data_ = (uint64_t(1) << n) - 1;
  }

  bool Get(size_t i) const { return (data_ >> i) & 1; }

  void operator|=(const FBitset& rhs) { data_ |= rhs.data_; }
  void operator&=(const FBitset& rhs) { data_ &= rhs.data_; }

  void TurnOff  (const FBitset& rhs)                        { data_ &= ~rhs.data_; }
  void SetNegAnd(const FBitset& rhs1, const FBitset& rhs2)  { data_ = (~rhs1.data_) & rhs2.data_; }
  void SetAnd   (const FBitset& rhs1, const FBitset& rhs2)  { data_ = rhs1.data_ & rhs2.data_; }

  bool Subsumes(const FBitset& other) const { return (data_ | other.data_) == data_; }

  std::vector<int> Elements() const {
    std::vector<int> ret;
    uint64_t td = data_;
    while (td) { ret.push_back(__builtin_ctzll(td)); td &= ~-td; }
    return ret;
  }

  int Popcount() const { return __builtin_popcountll(data_); }

  bool Intersects(const FBitset& other) const          { return data_ & other.data_; }
  int IntersectionPopcount(const FBitset& other) const { return __builtin_popcountll(data_ & other.data_); }

  // Returns BITS when empty.
  int First() const { return data_ ? __builtin_ctzll(data_) : BITS; }

  class FBitsetIterator {
    uint64_t tb_;
   public:
    explicit FBitsetIterator(uint64_t tb) : tb_(tb) {}
    bool operator!=(const FBitsetIterator& o) const { return tb_ != o.tb_; }
    const FBitsetIterator& operator++() { tb_ &= ~-tb_; return *this; }
    int operator*() const { return __builtin_ctzll(tb_); }
  };

  FBitsetIterator begin() const { return FBitsetIterator(data_); }
  FBitsetIterator end()   const { return FBitsetIterator(0); }
};

// Open-addressed hash set of FBitset. Uses data_==0 as the empty-slot sentinel;
// inserting the zero bitset is therefore undefined (never needed in practice).
class FBitsetSet {
 public:
  FBitsetSet() {}
  FBitsetSet(size_t capacity, double load_factor) {
    load_factor_ = load_factor;
    assert(load_factor_ >= 1.1);
    capacity_ = NextPrime((capacity + 1) * load_factor_);
    assert((size_t)(capacity_ * load_factor_) > capacity_);
    container_.resize(capacity_, 0);
  }
  bool Insert(const FBitset& bitset) {
    size_t ind = Hash(bitset.data_, capacity_);
    while (1) {
      if (container_[ind] == 0)              break;
      if (container_[ind] == bitset.data_)   return false;
      if (++ind == capacity_) ind = 0;
    }
    container_[ind] = bitset.data_;
    elements_++;
    if ((size_t)(elements_ * load_factor_) > capacity_) {
      Resize();
      assert((size_t)(elements_ * load_factor_) < capacity_);
    }
    return true;
  }
  bool Inited() const { return capacity_ > 0; }
  size_t ContainerSize() const { return container_.size(); }
 private:
  size_t elements_ = 0;
  double load_factor_ = 0;
  size_t capacity_ = 0;
  std::vector<uint64_t> container_;

  bool IsPrime(size_t n) const {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0) return false;
    for (size_t i = 3; i*i <= n; i += 2) if (n % i == 0) return false;
    return true;
  }
  size_t NextPrime(size_t n) const { while (!IsPrime(n)) n++; return n; }
  size_t Hash(uint64_t data, size_t mod) const { return (data * 65599ULL) % (uint64_t)mod; }

  void Resize() {
    size_t new_capacity = NextPrime((capacity_ * 3) / 2);
    std::vector<uint64_t> nc(new_capacity, 0);
    for (size_t i = 0; i < capacity_; i++) {
      if (container_[i] == 0) continue;
      size_t ind = Hash(container_[i], new_capacity);
      while (nc[ind] != 0) { if (++ind == new_capacity) ind = 0; }
      nc[ind] = container_[i];
    }
    container_ = std::move(nc);
    capacity_ = new_capacity;
  }
};

// Open-addressed hash map from FBitset to int. Same zero-sentinel rule as FBitsetSet.
class FBitsetMap {
 public:
  FBitsetMap() {}
  FBitsetMap(size_t capacity, double load_factor) {
    load_factor_ = load_factor;
    assert(load_factor_ >= 1.1);
    capacity_ = NextPrime((capacity + 1) * load_factor_);
    assert((size_t)(capacity_ * load_factor_) > capacity_);
    container_.resize(capacity_, 0);
    values_.resize(capacity_, 0);
  }
  std::pair<int, bool> Insert(const FBitset& bitset, int value, bool replace=false) {
    assert(value > 0);
    size_t ind = Hash(bitset.data_, capacity_);
    while (1) {
      if (container_[ind] == 0) break;
      if (container_[ind] == bitset.data_) {
        assert(values_[ind] > 0);
        if (replace) values_[ind] = value;
        return {values_[ind], false};
      }
      if (++ind == capacity_) ind = 0;
    }
    container_[ind] = bitset.data_;
    values_[ind] = value;
    elements_++;
    if ((size_t)(elements_ * load_factor_) > capacity_) {
      Resize();
      assert((size_t)(elements_ * load_factor_) < capacity_);
    }
    return {value, true};
  }
  int Get(const FBitset& bitset, bool expect) const {
    size_t ind = Hash(bitset.data_, capacity_);
    while (1) {
      if (container_[ind] == 0) { assert(!expect); return 0; }
      if (container_[ind] == bitset.data_) { assert(values_[ind] > 0); return values_[ind]; }
      if (++ind == capacity_) ind = 0;
    }
    assert(0);
  }
  bool Inited() const { return capacity_ > 0; }
  size_t ContainerSize() const { return container_.size(); }
 private:
  size_t elements_ = 0;
  double load_factor_ = 0;
  size_t capacity_ = 0;
  std::vector<uint64_t> container_;
  std::vector<int> values_;

  bool IsPrime(size_t n) const {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0) return false;
    for (size_t i = 3; i*i <= n; i += 2) if (n % i == 0) return false;
    return true;
  }
  size_t NextPrime(size_t n) const { while (!IsPrime(n)) n++; return n; }
  size_t Hash(uint64_t data, size_t mod) const { return (data * 65599ULL) % (uint64_t)mod; }

  void Resize() {
    size_t new_capacity = NextPrime((capacity_ * 3) / 2);
    std::vector<uint64_t> nc(new_capacity, 0);
    std::vector<int> nv(new_capacity, 0);
    for (size_t i = 0; i < capacity_; i++) {
      if (container_[i] == 0) continue;
      size_t ind = Hash(container_[i], new_capacity);
      while (nc[ind] != 0) { if (++ind == new_capacity) ind = 0; }
      nc[ind] = container_[i];
      assert(values_[i] > 0);
      nv[ind] = values_[i];
    }
    container_ = std::move(nc);
    values_ = std::move(nv);
    capacity_ = new_capacity;
  }
};


class FLBSieve {
 public:
  FLBSieve() {}
  FLBSieve(size_t len, int k) : len_(len) {
    assert(len_ <= BITS);
    assert(k >= 0);
    containers_.resize(k+1);
    masks_.resize(k+1);
    elements_.resize(k+1);
    for (int i = 0; i <= k; i++) {
      containers_[i].push_back({});
    }
  }
  void Insert(const FBitset& bs, int lb) {
    assert(lb < (int)masks_.size() && lb >= 0);
    maxlb_ = std::max(maxlb_, lb);
    int mask = GetMask(lb, bs);
    containers_[lb][mask].push_back(bs.data_);
    elements_[lb]++;
    if (elements_[lb] > (1 << (2*(int)masks_[lb].size()))) {
      Resize(lb);
      assert(elements_[lb] <= (1 << (2*(int)masks_[lb].size())));
    }
  }
  int Get(const FBitset& bs, int k) {
    for (int i = k; i <= maxlb_; i++) {
      int mask = GetMask(i, bs);
      if (GetCont(i, bs, 0)) return i;
      for (int sub = 0; (sub = (sub - mask) & mask);) {
        if (GetCont(i, bs, sub)) return i;
      }
    }
    return 0;
  }
  size_t TotElements() const {
    size_t ret = 0;
    for (size_t e : elements_) ret += e;
    return ret;
  }
 private:
  int maxlb_ = 0;
  size_t len_ = 0;
  std::vector<size_t> elements_;
  std::vector<std::vector<FBitset>> masks_;
  std::vector<std::vector<std::vector<uint64_t>>> containers_;

  int GetMask(int lb, const FBitset& bs) {
    int mask = 0;
    for (int i = 0; i < (int)masks_[lb].size(); i++) {
      if (bs.Intersects(masks_[lb][i])) mask |= (1 << i);
    }
    return mask;
  }
  bool GetCont(int lb, const FBitset& bs, int cont) {
    for (uint64_t entry : containers_[lb][cont]) {
      if ((bs.data_ | entry) == bs.data_) return true;
    }
    return false;
  }
  int NIntersect(int lb, const FBitset& bs) {
    int n = 0;
    for (int cont = 0; cont < (int)containers_[lb].size(); cont++) {
      for (uint64_t entry : containers_[lb][cont]) {
        if (bs.data_ & entry) n++;
      }
    }
    return n;
  }
  void Resize(int lb) {
    int nmasks = masks_[lb].size() + 1;
    masks_[lb].clear();
    int ib = len_ / nmasks;
    assert(nmasks * ib <= (int)len_);
    assert(ib >= 2);
    int sp = 0;
    FBitset mask;
    for (int i = 0; i < nmasks; i++) {
      int ep = (i+1)*ib;
      assert(sp + ib <= ep);
      assert(ep <= (int)len_);
      mask.Clear();
      for (int j = sp; j < ep; j++) {
        mask.SetTrue(j);
        int ic = NIntersect(lb, mask);
        if (ic >= (int)elements_[lb]/2 || j+1 == ep) {
          masks_[lb].push_back(mask);
          sp = ep;
          break;
        }
      }
    }
    assert((int)masks_[lb].size() == nmasks);
    std::vector<int> cnts(1 << masks_[lb].size());
    for (int cont = 0; cont < (int)containers_[lb].size(); cont++) {
      for (uint64_t entry : containers_[lb][cont]) {
        int tmask = 0;
        for (int k = 0; k < (int)masks_[lb].size(); k++) {
          if (masks_[lb][k].data_ & entry) tmask |= (1 << k);
        }
        cnts[tmask]++;
      }
    }
    std::vector<std::vector<uint64_t>> new_containers(cnts.size());
    for (int i = 0; i < (int)cnts.size(); i++) {
      new_containers[i].reserve(cnts[i] * 2);
    }
    for (int cont = 0; cont < (int)containers_[lb].size(); cont++) {
      for (uint64_t entry : containers_[lb][cont]) {
        int tmask = 0;
        for (int k = 0; k < (int)masks_[lb].size(); k++) {
          if (masks_[lb][k].data_ & entry) tmask |= (1 << k);
        }
        new_containers[tmask].push_back(entry);
      }
    }
    containers_[lb] = new_containers;
    for (int i = 0; i < (int)cnts.size(); i++) {
      assert(containers_[lb][i].size() == (size_t)cnts[i]);
    }
  }
};

} // namespace sms