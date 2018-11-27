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
    using DescendantsCollection = std::array<std::optional<size_t>, 2 * TDegree + 1>;
    using KeysCollection = std::array<std::optional<TKey>, 2 * TDegree>;
    //using BytesArray = std::array<uint8_t, InnerNode::SizeOf()>;

public:
    InnerNode(size_t fileOffset, std::fstream &fileHandle, std::shared_ptr<Base> const &parent = nullptr);
    DescendantsCollection descendants;
    KeysCollection keys;
    static size_t BytesSize();
    std::pair<std::vector<TKey>, std::vector<size_t>> getEntries() const;
    InnerNode &setEntries(std::pair<std::vector<TKey>, std::vector<size_t>> const &entries);
    InnerNode &setKeys(typename std::vector<TKey>::iterator begIt, typename std::vector<TKey>::iterator endIt);
    InnerNode &
    setDescendants(typename std::vector<size_t>::iterator begIt, typename std::vector<size_t>::iterator endIt);
    TKey getKeyBetweenPointers(size_t aPtr, size_t bPtr);
    bool full() const override;
    InnerNode &add(TKey const &key, size_t descendantOffset);
public:
    InnerNode &changeKey(size_t aPtr, size_t bPtr, TKey const &key);
protected:
    size_t fillElementsSize() const override;
public:

    ~InnerNode() override;
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
    this->changed = true;
    auto descendantsBytePtr = (std::array<uint8_t, sizeof(this->descendants)> *) this->descendants.data();
    auto keysBytePtr = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin(), keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + keysBytePtr->size(), descendantsBytePtr->size(), descendantsBytePtr->begin());
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
    return BytesSize();
}

template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree>::InnerNode(size_t fileOffset, std::fstream &fileHandle,
                                            std::shared_ptr<InnerNode::Base> const &parent)
        :Base(fileOffset, fileHandle, parent) {}

template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree>::~InnerNode() {
    this->unload();
}

template<typename TKey, typename TValue, size_t TDegree>
size_t InnerNode<TKey, TValue, TDegree>::BytesSize() {
    return sizeof(DescendantsCollection) + sizeof(KeysCollection);
}

template<typename TKey, typename TValue, size_t TDegree>
size_t InnerNode<TKey, TValue, TDegree>::fillElementsSize() const {
    return static_cast<size_t>(std::count_if(this->descendants.begin(), this->descendants.end(),
                                             [](auto x) { return x != std::nullopt; }));
}

template<typename TKey, typename TValue, size_t TDegree>
bool InnerNode<TKey, TValue, TDegree>::full() const {
    return !static_cast<bool>(std::find(this->descendants.begin(), this->descendants.end(), std::nullopt));
}

template<typename TKey, typename TValue, size_t TDegree>
std::pair<std::vector<TKey>, std::vector<size_t>>
InnerNode<TKey, TValue, TDegree>::getEntries() const {
    auto result = std::pair<std::vector<TKey>, std::vector<size_t>>();
    for (auto &key : this->keys) {
        if (!key) break;
        result.first.push_back(*key);
    }
    for (auto &desc : this->descendants) {
        if (!desc) break;
        result.second.push_back(*desc);
    }
    return std::move(result);
}

template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::setEntries(std::pair<std::vector<TKey>, std::vector<size_t>> const &entries) {
    this->changed = true;
    this->keys = KeysCollection();
    this->descendants = DescendantsCollection();
    std::copy(entries.first.begin(), entries.first.end(), this->keys.begin());
    std::copy(entries.second.begin(), entries.second.end(), this->descendants.begin());
    return *this;
}
template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::changeKey(size_t aPtr, size_t bPtr, TKey const &key) {
    this->changed = true;
    auto beginWith = std::min(aPtr, bPtr);
    auto foundIter = std::find(this->descendants.begin(), this->descendants.end(), key);
    if (foundIter == this->descendants.end()) throw std::runtime_error("Couldnt find given ptr in parent!");
    this->keys[this->descendants.begin() - foundIter + 1] = key;
    return *this;
}
template<typename TKey, typename TValue, size_t TDegree>
TKey InnerNode<TKey, TValue, TDegree>::getKeyBetweenPointers(size_t aPtr, size_t bPtr) {
    auto beginWith = std::min(aPtr, bPtr);
    auto foundIter = std::find(this->descendants.begin(), this->descendants.end(), beginWith);
    if (foundIter == this->descendants.end()) throw std::runtime_error("Couldnt find given ptr in parent!");
    auto result = this->keys[this->descendants.begin() - foundIter + 1];
    if (!result) throw std::runtime_error("Searched pointer does not exists");
    return *result;
}
template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::setKeys(typename std::vector<TKey>::iterator begIt,
                                          typename std::vector<TKey>::iterator endIt) {
    this->changed = true;
    this->keys = KeysCollection();
    std::copy(begIt, endIt, this->keys.begin());
    return *this;
}

template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::setDescendants(typename std::vector<size_t>::iterator begIt,
                                                 typename std::vector<size_t>::iterator endIt) {
    this->changed = true;
    this->descendants = DescendantsCollection();
    std::copy(begIt, endIt, this->descendants.begin());

    return *this;
}

template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &InnerNode<TKey, TValue, TDegree>::add(TKey const &key, size_t descendantOffset) {
    if (this->full()) throw std::runtime_error("Unable to add new key, desc to full node");
    this->changed = true;
    std::pair<std::vector<TKey>, std::vector<size_t>> data = this->getEntries();
    auto insertPosition =  std::upper_bound(data.first.begin(), data.first.end(), key) - data.first.begin();
    data.first.insert(data.first.begin() + insertPosition, key);
    data.second.insert(data.second.begin() + insertPosition + 1, descendantOffset);
    this->setEntries(data);
    return *this;
}


#endif //SBD2_INNER_NODE_HH
