//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_INNER_NODE_HH
#define SBD2_INNER_NODE_HH

#include <optional>
#include "node.hh"

template<typename TKey, typename TValue, size_t TDegree> struct InnerNode final : Node<TKey, TValue> {
    std::array<std::optional<Node<TKey, TValue>>, 2 * TDegree + 1> descendants;
    std::array<std::optional<TKey>, 2 * TDegree> keys;
};

#endif //SBD2_INNER_NODE_HH
