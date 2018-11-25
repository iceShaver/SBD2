//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_INNER_NODE_HH
#define SBD2_INNER_NODE_HH

#include <optional>
#include <bitset>
#include "node.hh"

template<typename TKey, typename TValue, size_t TDegree> class InnerNode final : public Node<TKey, TValue> {
    using Base = Node<TKey, TValue>;
    using BytesArray = std::array<uint8_t, InnerNode::SizeOf()>;

public:
    std::array<std::optional<size_t>, 2 * TDegree + 1> descendants;
    std::array<std::optional<TKey>, 2 * TDegree> keys;

private:
    std::ostream &printData(std::ostream &o) const override;

    Node<TKey, TValue> &deserialize(std::vector<uint8_t> const &bytes) override;
    std::vector<uint8_t> getData() override;

    NodeType nodeType() const override;
    size_t bytesSize() const override;
    size_t elementsSize() const override;
    constexpr auto ElementsSize() const noexcept;
};


template<typename TKey, typename TValue, size_t TDegree>
constexpr auto InnerNode<TKey, TValue, TDegree>::ElementsSize() const noexcept {
    return this->descendants.size() + this->keys.size() + 1;
}

template<typename TKey, typename TValue, size_t TDegree> NodeType InnerNode<TKey, TValue, TDegree>::nodeType() const {
    return NodeType::INNER;
}

template<typename TKey, typename TValue, size_t TDegree>
std::vector<uint8_t> InnerNode<TKey, TValue, TDegree>::getData() {
    auto result = std::vector<uint8_t>();
    auto keysByteArray = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    auto descendantsByteArray = (std::array<uint8_t, sizeof(this->descendants)> *) this->descendants.data();
    result.reserve(keysByteArray->size() + descendantsByteArray->size());
    std::copy(keysByteArray->begin(), keysByteArray->end(), std::back_inserter(result));
    std::copy(descendantsByteArray->begin(), descendantsByteArray->end(), std::back_inserter(result));
    return std::move(result);
}

template<typename TKey, typename TValue, size_t TDegree> size_t InnerNode<TKey, TValue, TDegree>::elementsSize() const {
    return ElementsSize();
}

template<typename TKey, typename TValue, size_t TDegree>
Node<TKey, TValue> &InnerNode<TKey, TValue, TDegree>::deserialize(std::vector<uint8_t> const &bytes) {
    auto header = static_cast<std::bitset<8>>(bytes[0]);
    if (header[0] == true) // is empty
        throw std::runtime_error("Read empty page");
    if (header[1] != static_cast<int>(NodeType::INNER))
        throw std::runtime_error("Read bad page");
    auto descendantsBytePtr = (std::array<uint8_t, sizeof(this->descendants)> *) this->descendants.data();
    auto keysBytePtr = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin() + 1, keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + 1 + keysBytePtr->size(), descendantsBytePtr->size(), descendantsBytePtr->begin());
    return *this;
}

template<typename TKey, typename TValue, size_t TDegree>
std::ostream &InnerNode<TKey, TValue, TDegree>::printData(std::ostream &o) const {
    int i = 0;
    std::optional<TKey> key;
    std::optional<size_t> descendant;
    while ((key = this->keys[i]) && (descendant = this->descendants[i])) {
        o << "D:" << *descendant << ' ' << "K:" << *key << ' ';
        ++i;
    }
    o << "D:" << *this->descendants[i + 1] << '\n';
    return o.flush();
}

template<typename TKey, typename TValue, size_t TDegree> size_t InnerNode<TKey, TValue, TDegree>::bytesSize() const {
    return sizeof(this->keys) + sizeof(this->descendants) + 1;
}


#endif //SBD2_INNER_NODE_HH
