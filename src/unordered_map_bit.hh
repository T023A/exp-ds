//
// Created by pawan on 3/18/21.
//

#ifndef UNORDERED_MAP_BIT_HH
#define UNORDERED_MAP_BIT_HH


#include <cstdint>




/* usage: typedef ds::ul_map<64,22> ul_map_t; // you may choose to use more or less bits, more will increase memory usage.
 * ul_map_t::insert(int64_t)
 * ul_map_t::erase(int64_t)
 * ul_map_t::exists(int64_t)
 * */

namespace ds {

    static constexpr int64_t sentinel_value = -1;

    struct data_t {
        data_t() : value(0) {
        }

        data_t(int v) : value(v) {
        }

        int64_t value;
    };


    struct ul_prefix_node_chunk_t {
        ul_prefix_node_chunk_t() : ptrNextChunk(nullptr) {
            for (uint8_t i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
                numbers[i] = sentinel_value;
            }
        }


        static constexpr uint8_t MAX_COUNT = 4; // this can be tuned as well
        ul_prefix_node_chunk_t *ptrNextChunk;
        int64_t numbers[MAX_COUNT];
    };

    static_assert(sizeof(ul_prefix_node_chunk_t) <= std::hardware_destructive_interference_size);

    struct alignas(std::hardware_destructive_interference_size) ul_prefix_node_t {
        ul_prefix_node_t() : totalCount(0) {
        }

        bool addNum(int64_t n) {
            for (ul_prefix_node_chunk_t *ptrChunk = &headChunk; ptrChunk; ptrChunk = ptrChunk->ptrNextChunk) {
                auto &numbers = ptrChunk->numbers;

                for (int i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
                    if (numbers[i] == sentinel_value) {
                        numbers[i] = n;
                        ++totalCount;
                        return true;
                    }
                    else if (numbers[i] == n) {
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
                delete(ptrHeapChunk);
                ptrHeapChunk = t;
            }
        }

        int32_t totalCount;
        ul_prefix_node_chunk_t headChunk;
    };


    static constexpr int NUM_BITS_PER_BYTE = 8;


    template<int maxBits, int numBitsPerNode>
    struct ul_layout_t {
        static_assert(maxBits <= 64);

        ul_layout_t() {
        }

        const inline ul_prefix_node_t *getNode(int8_t idxUnit, int32_t unitBytes) const {
            return nodes[idxUnit * numNodesPerUnit + unitBytes];
        }

        inline ul_prefix_node_t &getNode(int8_t idxUnit, int32_t unitBytes) {
            auto &n = nodes[idxUnit * numNodesPerUnit + unitBytes];


            return n;
        }

        inline const ul_prefix_node_t &getNode(int32_t offset) const {
            return nodes[offset];
        }

        inline ul_prefix_node_t &getNode(int32_t offset) {
            return nodes[offset];
        }


        static constexpr int64_t numNodesPerUnit = 1LL << (numBitsPerNode);
        static constexpr int64_t numBitsPerUnit = numBitsPerNode;
        static constexpr int64_t unitBitMask = (1LL << (numBitsPerNode)) - 1;
        static constexpr int64_t numUnits = maxBits / numBitsPerNode;
        static constexpr int64_t partialUnitBytes = 1LL << (maxBits % numBitsPerNode);
        static constexpr int64_t numNodes = numUnits * numNodesPerUnit;
        ul_prefix_node_t nodes[numNodes];
    };


    template<int maxBits = 64, int numBitsPerNode = 24>
    struct ul_map {
        typedef ul_layout_t<maxBits, numBitsPerNode> node_t;
        static constexpr int MAX_BITS = maxBits;
        static constexpr int NUM_BITS_PER_NODE = numBitsPerNode;

        ul_map() : buckets() {
        }

        bool exists(const int64_t num) {
            const auto &node = buckets.getNode(num & node_t::unitBitMask);
            const ul_prefix_node_t *ptrMinNodeList = &node;

            for (uint8_t i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
                if (node.headChunk.numbers[i] == num) { return true; }
            }

            if (!node.headChunk.ptrNextChunk) return false;

            const ul_prefix_node_chunk_t *searchChunk = ptrMinNodeList->headChunk.ptrNextChunk;

            int64_t num2 = num >> node_t::numBitsPerUnit;

            for (int unit_idx = 1, offset = node_t::numNodesPerUnit;
                 unit_idx < node_t::numUnits; ++unit_idx, offset += node_t::numNodesPerUnit) {
                const int32_t digit = num2 & node_t::unitBitMask;
                const ul_prefix_node_t &nn = buckets.getNode(offset + digit);

                if (!nn.totalCount) return false;
                else if (ptrMinNodeList->totalCount > nn.totalCount) {
                    ptrMinNodeList = &nn;
                    searchChunk = &nn.headChunk;
                }

                num2 >>= node_t::numBitsPerUnit;
            }


            for (const ul_prefix_node_chunk_t *ptrChunk = searchChunk; ptrChunk; ptrChunk = ptrChunk->ptrNextChunk) {
                for (int i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
                    if (ptrChunk->numbers[i] == num) {
                        return true;
                    }
                }
            }

            return false;
        }

        int erase(int64_t num) {
            int64_t num2 = num;

            for (int unit_idx = 0, offset = 0;
                 unit_idx < node_t::numUnits; ++unit_idx, offset += node_t::numNodesPerUnit) {
                ul_prefix_node_t &nn = buckets.getNode(offset + (num2 & node_t::unitBitMask));


                for (ul_prefix_node_chunk_t *ptrChunk = &nn.headChunk; ptrChunk; ptrChunk = ptrChunk->ptrNextChunk) {
                    auto &nns = ptrChunk->numbers;
                    for (int i = 0; i < ul_prefix_node_chunk_t::MAX_COUNT; ++i) {
                        if (nns[i] == num) {
                            nns[i] = sentinel_value;
                            num2 >>= node_t::numBitsPerUnit;
                            goto next;
                        }
                    }
                }
                return 0;
            next:
                --nn.totalCount;
            }
        done:
            return 1;
        }

        bool insert(int64_t num) {
            int64_t num2 = num;

            for (int unit_idx = 0; unit_idx < buckets.numUnits; ++unit_idx) {
                const int digit = num2 & buckets.unitBitMask;

                if (ul_prefix_node_t &nn = buckets.getNode(unit_idx, digit); !nn.addNum(num)) {
                    return false;
                }
                num2 >>= buckets.numBitsPerUnit;
            }
            return true;
        }

        ul_layout_t<maxBits, numBitsPerNode> buckets;
    };
}


#endif //UNORDERED_MAP_BIT_HH
