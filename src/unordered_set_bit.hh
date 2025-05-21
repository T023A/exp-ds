/**
MIT License

Copyright (c) 2025 Pawan Chawla (pawan.s.chawla@outlook.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


*/

#ifndef UNORDERED_SET_BIT_IMPL_HH
#define UNORDERED_SET_BIT_IMPL_HH

#include <cstdint>

/* usage: typedef ds::ul_set<64,22> ul_set; // you may choose to use more or
 * less bits, more will increase memory usage. ul_set::insert(int64_t)
 * ul_set::erase(int64_t)
 * ul_set::find(int64_t)
 * */

namespace ds {
namespace internal {
static constexpr int64_t sentinel_value = -1;

struct data_t {
  typedef int64_t data_type;

  data_t() : value(0) {}

  data_t(data_type v) : value(v) {}

  data_type value;
};

struct ul_prefix_node_chunk_t {
  ul_prefix_node_chunk_t() : ptrNextChunk(nullptr) {
    for (uint8_t i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
      numbers[i] = sentinel_value;
    }
  }

  static constexpr uint8_t MAX_COUNT = 4;  // this can be tuned as well
  ul_prefix_node_chunk_t *ptrNextChunk;
  int64_t numbers[MAX_COUNT];
};

static_assert(sizeof(ul_prefix_node_chunk_t) <=
              std::hardware_destructive_interference_size);

struct alignas(std::hardware_destructive_interference_size) ul_prefix_node_t {
  ul_prefix_node_t() : totalCount(0) {}

  bool addNum(int64_t n) {
    for (ul_prefix_node_chunk_t *ptrChunk = &headChunk; ptrChunk;
         ptrChunk = ptrChunk->ptrNextChunk) {
      auto &numbers = ptrChunk->numbers;

      for (int i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
        if (numbers[i] == sentinel_value) {
          numbers[i] = n;
          ++totalCount;
          return true;
        } else if (numbers[i] == n) {
          return false;
        }
      }

      if (!ptrChunk->ptrNextChunk) {
        auto *newChunk = new ul_prefix_node_chunk_t();
        ptrChunk->ptrNextChunk = newChunk;
        newChunk->numbers[0] = n;
        ++totalCount;
        return true;
      }
    }

    return false;
  }

  ~ul_prefix_node_t() {
    auto *ptrHeapChunk = headChunk.ptrNextChunk;

    while (ptrHeapChunk) {
      auto t = ptrHeapChunk->ptrNextChunk;
      delete (ptrHeapChunk);
      ptrHeapChunk = t;
    }
  }

  int32_t totalCount;
  ul_prefix_node_chunk_t headChunk;
};

static constexpr int NUM_BITS_PER_BYTE = 8;

template <int maxBits, int numBitsPerNode>
struct ul_layout_t {
  static_assert(maxBits <= 64);

  ul_layout_t() {}

  const inline ul_prefix_node_t *getNode(int8_t idxUnit,
                                         int32_t unitBytes) const {
    return nodes[idxUnit * numNodesPerUnit + unitBytes];
  }

  inline ul_prefix_node_t &getNode(int8_t idxUnit, int32_t unitBytes) {
    auto &n = nodes[idxUnit * numNodesPerUnit + unitBytes];

    return n;
  }

  inline const ul_prefix_node_t &getNode(int32_t offset) const {
    return nodes[offset];
  }

  inline ul_prefix_node_t &getNode(int32_t offset) { return nodes[offset]; }

  static constexpr int64_t numNodesPerUnit = 1LL << (numBitsPerNode);
  static constexpr int64_t numBitsPerUnit = numBitsPerNode;
  static constexpr int64_t unitBitMask = (1LL << (numBitsPerNode)) - 1;
  static constexpr int64_t numUnits = maxBits / numBitsPerNode;
  static constexpr int64_t partialUnitBytes = 1LL << (maxBits % numBitsPerNode);
  static constexpr int64_t numNodes = numUnits * numNodesPerUnit;
  ul_prefix_node_t nodes[numNodes];
};
}  // namespace internal

template <int maxBits = 64, int numBitsPerNode = 24>
class ul_set {
 private:
  typedef internal::ul_layout_t<maxBits, numBitsPerNode> node_t;
  static constexpr int MAX_BITS = maxBits;
  static constexpr int NUM_BITS_PER_NODE = numBitsPerNode;

 public:
  typedef internal::data_t::data_type key_type;
  typedef internal::data_t::data_type value_type;

 public:
  ul_set() : buckets() {}

  bool find(const int64_t num) const {
    const auto &node = buckets.getNode(num & node_t::unitBitMask);
    const internal::ul_prefix_node_t *ptrMinNodeList = &node;

    for (uint8_t i = 0; i < internal::ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
      if (node.headChunk.numbers[i] == num) {
        return true;
      }
    }

    if (!node.headChunk.ptrNextChunk) return false;

    const internal::ul_prefix_node_chunk_t *searchChunk =
        ptrMinNodeList->headChunk.ptrNextChunk;

    int64_t num2 = num >> node_t::numBitsPerUnit;

    for (int unit_idx = 1, offset = node_t::numNodesPerUnit;
         unit_idx < node_t::numUnits;
         ++unit_idx, offset += node_t::numNodesPerUnit) {
      const int32_t digit = num2 & node_t::unitBitMask;
      const internal::ul_prefix_node_t &nn = buckets.getNode(offset + digit);

      if (!nn.totalCount)
        return false;
      else if (ptrMinNodeList->totalCount > nn.totalCount) {
        ptrMinNodeList = &nn;
        searchChunk = &nn.headChunk;
      }

      num2 >>= node_t::numBitsPerUnit;
    }

    for (const internal::ul_prefix_node_chunk_t *ptrChunk = searchChunk;
         ptrChunk; ptrChunk = ptrChunk->ptrNextChunk) {
      for (int i = 0; i < internal::ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
        if (ptrChunk->numbers[i] == num) {
          return true;
        }
      }
    }

    return false;
  }

  bool erase(int64_t num) {
    int64_t num2 = num;

    for (int unit_idx = 0, offset = 0; unit_idx < node_t::numUnits;
         ++unit_idx, offset += node_t::numNodesPerUnit) {
      internal::ul_prefix_node_t &nn =
          buckets.getNode(offset + (num2 & node_t::unitBitMask));

      for (internal::ul_prefix_node_chunk_t *ptrChunk = &nn.headChunk; ptrChunk;
           ptrChunk = ptrChunk->ptrNextChunk) {
        auto &nns = ptrChunk->numbers;
        for (int i = 0; i < internal::ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
          if (nns[i] == num) {
            nns[i] = internal::sentinel_value;
            num2 >>= node_t::numBitsPerUnit;
            goto next;
          }
        }
      }
      return false;
    next:
      --nn.totalCount;
    }
  done:
    return true;
  }

  bool insert(int64_t num) {
    int64_t num2 = num;

    for (int unit_idx = 0; unit_idx < buckets.numUnits; ++unit_idx) {
      const int digit = num2 & buckets.unitBitMask;

      if (internal::ul_prefix_node_t &nn = buckets.getNode(unit_idx, digit);
          !nn.addNum(num)) {
        return false;
      }
      num2 >>= buckets.numBitsPerUnit;
    }
    return true;
  }

 private:
  internal::ul_layout_t<maxBits, numBitsPerNode> buckets;
};
}  // namespace ds

#endif  // UNORDERED_SET_BIT_IMPL_HH
