//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_LEAF_NODE_HH
#define SBD2_LEAF_NODE_HH

#include <optional>
#include <array>
#include "node.hh"

template<typename TKey, typename TValue, size_t TDegree> class LeafNode final : public Node<TKey, TValue> {
    using Base = Node<TKey, TValue>;

public:
    LeafNode(size_t fileOffset, std::fstream &fileHandle, std::weak_ptr<Base> const &parent = std::weak_ptr<Base>());
    std::array<std::optional<TKey>, 2 * TDegree> keys;
    std::array<std::optional<TValue>, 2 * TDegree> values;

    ~LeafNode() override;
private:
    std::ostream &printData(std::ostream &o) const override;
    Node<TKey, TValue> &deserialize(std::vector<uint8_t> const &bytes) override;
    std::vector<uint8_t> getData() override;
protected:
    size_t bytesSize() const override;
private:
    size_t elementsSize() const override;
    NodeType nodeType() const override;;
    constexpr auto ElementsSize() const noexcept;

};


template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree>::LeafNode(size_t fileOffset, std::fstream &fileHandle,
                                          std::weak_ptr<Base> const &parent) : Base(fileOffset, fileHandle, parent) {}

template<typename TKey, typename TValue, size_t TDegree> size_t LeafNode<TKey, TValue, TDegree>::elementsSize() const {
    return ElementsSize();
}

template<typename TKey, typename TValue, size_t TDegree>
constexpr auto LeafNode<TKey, TValue, TDegree>::ElementsSize() const noexcept {
    return this->keys.size() + this->values.size() + 1;
}

template<typename TKey, typename TValue, size_t TDegree>
NodeType LeafNode<TKey, TValue, TDegree>::nodeType() const { return NodeType::LEAF; }

template<typename TKey, typename TValue, size_t TDegree>
std::vector<uint8_t> LeafNode<TKey, TValue, TDegree>::getData() {
    auto result = std::vector<uint8_t>();
    auto keysByteArray = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    auto valuesByteArray = (std::array<uint8_t, sizeof(this->values)> *) (this->values.data());
    result.reserve(keysByteArray->size() + valuesByteArray->size());
    std::copy(keysByteArray->begin(), keysByteArray->end(), std::back_inserter(result));
    std::copy(valuesByteArray->begin(), valuesByteArray->end(), std::back_inserter(result));
    return std::move(result);
}

template<typename TKey, typename TValue, size_t TDegree>
Node<TKey, TValue> &LeafNode<TKey, TValue, TDegree>::deserialize(std::vector<uint8_t> const &bytes) {

    auto valuesBytePtr = (std::array<uint8_t, sizeof(this->values)> *) this->values.data();
    auto keysBytePtr = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin(), keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + keysBytePtr->size(), valuesBytePtr->size(), valuesBytePtr->begin());
    return *this;
}

template<typename TKey, typename TValue, size_t TDegree>
std::ostream &LeafNode<TKey, TValue, TDegree>::printData(std::ostream &o) const {
    int i = 0;
    auto key = this->keys[i];
    auto value = this->values[i];
    do o << "D:" << *key << ' ' << "K:" << *value << ' ';
    while (++i, (key = this->keys[i]) && (value = this->values[i]));
    return o;
}

template<typename TKey, typename TValue, size_t TDegree>
size_t LeafNode<TKey, TValue, TDegree>::bytesSize() const {
    return sizeof(this->keys) + sizeof(this->values);
}
template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree>::~LeafNode() {
        this->unload();
}


#endif //SBD2_LEAF_NODE_HH
