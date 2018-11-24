//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_LEAF_NODE_HH
#define SBD2_LEAF_NODE_HH

#include <optional>
#include "node.hh"

template<typename TKey, typename TValue, size_t TDegree> struct LeafNode final : Node<TKey, TValue> {
    std::array<std::optional<TKey>, 2 * TDegree> keys;
    std::array<std::optional<TValue>, 2 * TDegree> values;
};

#endif //SBD2_LEAF_NODE_HH
