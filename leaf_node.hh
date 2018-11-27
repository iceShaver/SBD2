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
    using KeysCollection = std::array<std::optional<TKey>, 2 * TDegree>;
    using ValuesCollection =  std::array<std::optional<TValue>, 2 * TDegree>;
public:
    LeafNode(size_t fileOffset, std::fstream &fileHandle, std::weak_ptr<Base> const &parent = std::weak_ptr<Base>());
    KeysCollection keys;
    ValuesCollection values;
    static size_t BytesSize();
    LeafNode &insert(TKey const &key, TValue const &value);
protected:
    size_t fillElementsSize() const override;
    bool isFull() const override;
public:

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

template<typename TKey, typename TValue, size_t TDegree>
size_t LeafNode<TKey, TValue, TDegree>::BytesSize() {
    return sizeof(KeysCollection) + sizeof(ValuesCollection);
}

template<typename TKey, typename TValue, size_t TDegree>
size_t LeafNode<TKey, TValue, TDegree>::fillElementsSize() const {
    return static_cast<size_t>(std::count_if(this->keys.begin(), this->keys.end(),
                                             [](auto x) { return x != std::nullopt; }));
}

template<typename TKey, typename TValue, size_t TDegree>
bool LeafNode<TKey, TValue, TDegree>::isFull() const {
    return !static_cast<bool>(std::find(this->keys.begin(), this->keys.end(), std::nullopt));
}

template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree> &LeafNode<TKey, TValue, TDegree>::insert(TKey const &key, TValue const &value) {
    if (this->isFull()) throw std::runtime_error("Tried to add element to full node");
    auto tmpKeys = std::vector<TKey>(this->keys);
    auto tmpVals = std::vector<TValue>(this->keys);
    auto posIterator = tmpKeys.insert(std::upper_bound(tmpKeys.begin(), tmpKeys.end(), key), key);
    auto position = std::distance(tmpKeys.begin(), posIterator);
    tmpVals.insert(position + position, value);
    this->keys = tmpKeys;
    this->values = tmpVals;
    return *this;
}


#endif //SBD2_LEAF_NODE_HH
