//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_NODE_HH
#define SBD2_NODE_HH

#include <cstddef>
#include <memory>
template<typename TKey,typename TValue> struct Node {
    std::weak_ptr<Node> parent;
};

#endif //SBD2_NODE_HH
