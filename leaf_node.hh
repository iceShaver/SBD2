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
    LeafNode(size_t fileOffset, std::fstream &fileHandle, std::shared_ptr<Base> parent = nullptr);
    KeysCollection keys;
    ValuesCollection values;
    static size_t BytesSize();
    LeafNode &insert(TKey const &key, TValue const &value);
    bool full() const override;
    std::vector<std::pair<TKey, TValue>> getRecords() const;
    LeafNode &setRecords(std::vector<std::pair<TKey, TValue>> const &records);
    LeafNode &setRecords(typename std::vector<std::pair<TKey, TValue>>::iterator it1,
                         typename std::vector<std::pair<TKey, TValue>>::iterator it2);

protected:
    size_t fillElementsSize() const override;
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
                                          std::shared_ptr<Base> parent) : Base(fileOffset, fileHandle, parent) {}

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
    this->changed = true;
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
    do o << "[K:" << *key << ' ' << "V:" << *value << "] ";
    while (++i, (key = this->keys[i]) && (value = this->values[i]));
    return o;
}

template<typename TKey, typename TValue, size_t TDegree>
size_t LeafNode<TKey, TValue, TDegree>::bytesSize() const {
    return sizeof(this->keys) + sizeof(this->values);
}

template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree>::~LeafNode() {
    this->unload(); // don't touch this!
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
bool LeafNode<TKey, TValue, TDegree>::full() const {
    return std::find(this->keys.begin(), this->keys.end(), std::nullopt) == this->keys.end();
}

template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree> &LeafNode<TKey, TValue, TDegree>::insert(TKey const &key, TValue const &value) {
    if (std::find(this->keys.begin(), this->keys.end(), key) != this->keys.end())
        throw std::runtime_error("Record with given key already exists");
    if (this->full()) throw std::runtime_error("Tried to add element to full node");
    this->changed = true;
    auto records = this->getRecords();
    records.insert(std::upper_bound(records.begin(), records.end(), std::pair(key, value),
                                    [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
    this->setRecords(records);
    return *this;
}

template<typename TKey, typename TValue, size_t TDegree>
std::vector<std::pair<TKey, TValue>>
LeafNode<TKey, TValue, TDegree>::getRecords() const {
    auto result = std::vector<std::pair<TKey, TValue>>();
    for (int i = 0; i < this->keys.size(); ++i) {
        if (!this->keys[i]) return result;
        result.emplace_back(*this->keys[i], *this->values[i]);
    }
    return std::move(result);
}

template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree> &
LeafNode<TKey, TValue, TDegree>::setRecords(std::vector<std::pair<TKey, TValue>> const &records) {
    this->changed = true;
    KeysCollection keys;
    ValuesCollection values;
    std::transform(records.begin(), records.end(), keys.begin(), [](auto x) { return x.first; });
    std::transform(records.begin(), records.end(), values.begin(), [](auto x) { return x.second; });
    this->keys = std::move(keys);
    this->values = std::move(values);
    return *this;
}

template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree> &
LeafNode<TKey, TValue, TDegree>::setRecords(typename std::vector<std::pair<TKey, TValue>>::iterator it1,
                                            typename std::vector<std::pair<TKey, TValue>>::iterator it2) {
    this->changed = true;
    KeysCollection keys;
    ValuesCollection values;
    std::transform(it1, it2, keys.begin(), [](auto x) { return x.first; });
    std::transform(it1, it2, values.begin(), [](auto x) { return x.second; });
    this->keys = std::move(keys);
    this->values = std::move(values);
    return *this;
}


#endif //SBD2_LEAF_NODE_HH
