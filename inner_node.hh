//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_INNER_NODE_HH
#define SBD2_INNER_NODE_HH

#include <optional>
#include <bitset>
#include "node.hh"

template<typename TKey, typename TValue, size_t TDegree>
class InnerNode final : public Node<TKey, TValue> {
    using Base = Node<TKey, TValue>;
    using DescendantsCollection = std::array<std::optional<size_t>, 2 * TDegree + 1>;
    using KeysCollection = std::array<std::optional<TKey>, 2 * TDegree>;

public:
    InnerNode(size_t fileOffset, std::fstream &fileHandle, std::shared_ptr<Base> const &parent = nullptr);
    ~InnerNode() override;

    static size_t BytesSize();
    std::pair<std::vector<TKey>, std::vector<size_t>> getEntries() const;
    InnerNode &setEntries(std::pair<std::vector<TKey>, std::vector<size_t>> const &entries);
    InnerNode &setKeys(typename std::vector<TKey>::iterator begIt, typename std::vector<TKey>::iterator endIt);
    InnerNode &setDescendants(typename std::vector<size_t>::iterator begIt,
                              typename std::vector<size_t>::iterator endIt);
    TKey getKeyAfterPointer(size_t aPtr);
    TKey compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const &key, TValue const &value,
                                          size_t nodeOffset) override;
    bool full() const override;
    InnerNode &add(TKey const &key, size_t descendantOffset);
    InnerNode &changeKey(size_t aPtr, TKey const &key);


    DescendantsCollection descendants;
    KeysCollection keys;

private:
    size_t fillElementsSize() const override;
    std::ostream &print(std::ostream &o) const override;
    Node<TKey, TValue> &deserialize(std::vector<uint8_t> const &bytes) override;
    std::vector<uint8_t> getData() override;
    NodeType nodeType() const override;
    size_t bytesSize() const override;
    size_t elementsSize() const override;
    constexpr auto ElementsSize() const noexcept;
};


template<typename TKey, typename TValue, size_t TDegree>
constexpr auto
InnerNode<TKey, TValue, TDegree>::ElementsSize() const noexcept {
    return this->descendants.size() + this->keys.size() + 1;
}


template<typename TKey, typename TValue, size_t TDegree>
NodeType
InnerNode<TKey, TValue, TDegree>::nodeType() const {
    return NodeType::INNER;
}


template<typename TKey, typename TValue, size_t TDegree>
std::vector<uint8_t>
InnerNode<TKey, TValue, TDegree>::getData() {
    auto result = std::vector<uint8_t>();
    auto keysByteArray = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    auto descendantsByteArray = (std::array<uint8_t, sizeof(this->descendants)> *) this->descendants.data();
    result.reserve(keysByteArray->size() + descendantsByteArray->size());
    std::copy(keysByteArray->begin(), keysByteArray->end(), std::back_inserter(result));
    std::copy(descendantsByteArray->begin(), descendantsByteArray->end(), std::back_inserter(result));
    return std::move(result);
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
InnerNode<TKey, TValue, TDegree>::elementsSize() const {
    return ElementsSize();
}


template<typename TKey, typename TValue, size_t TDegree>
Node<TKey, TValue> &
InnerNode<TKey, TValue, TDegree>::deserialize(std::vector<uint8_t> const &bytes) {
    this->changed = true;
    auto descendantsBytePtr = (std::array<uint8_t, sizeof(this->descendants)> *) this->descendants.data();
    auto keysBytePtr = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin(), keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + keysBytePtr->size(), descendantsBytePtr->size(), descendantsBytePtr->begin());
    return *this;
}


template<typename TKey, typename TValue, size_t TDegree>
std::ostream &
InnerNode<TKey, TValue, TDegree>::print(std::ostream &o) const {
    o << "INode: " << this->fileOffset << " { ";
    auto[keys, descendants] = this->getEntries();
    for (int i = 0; i < keys.size(); ++i) {
        o << "D:" << descendants[i] << ' ' << "K:" << keys[i] << ' ';
    }
    return o << "D:" << *descendants.rbegin() << '}';
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
InnerNode<TKey, TValue, TDegree>::bytesSize() const {
    return BytesSize();
}


template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree>::InnerNode(size_t fileOffset, std::fstream &fileHandle,
                                            std::shared_ptr<InnerNode::Base> const &parent) : Base(fileOffset,
                                                                                                   fileHandle,
                                                                                                   parent) {}


template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree>::~InnerNode() {
    this->unload(); // don't touch this!
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
InnerNode<TKey, TValue, TDegree>::BytesSize() {
    return sizeof(DescendantsCollection) + sizeof(KeysCollection);
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
InnerNode<TKey, TValue, TDegree>::fillElementsSize() const {
    return static_cast<size_t>(std::count_if(this->descendants.begin(), this->descendants.end(),
                                             [](auto x) { return x != std::nullopt; }));
}


template<typename TKey, typename TValue, size_t TDegree>
bool
InnerNode<TKey, TValue, TDegree>::full() const {
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


// TODO: this is nonsens
template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::changeKey(size_t aPtr, TKey const &key) {
    this->changed = true;
    auto foundIter = std::find(this->descendants.begin(), this->descendants.end(), aPtr);
    auto keyToChangeIndex = foundIter - this->descendants.begin();
    if (foundIter == this->descendants.end())
        throw std::runtime_error("Internal DB error: couldn't find given ptr in parent!");
    this->keys[keyToChangeIndex] = key;
    return *this;
}


// TODO: this is nonsens
template<typename TKey, typename TValue, size_t TDegree>
TKey
InnerNode<TKey, TValue, TDegree>::getKeyAfterPointer(size_t aPtr) {
    auto foundIter = std::find(this->descendants.begin(), this->descendants.end(), aPtr);
    if (foundIter == this->descendants.end())
        throw std::runtime_error("Internal DB error: couldn't find given ptr in parent!");
    auto keyIndex = foundIter - this->descendants.begin();
    if (!this->keys[keyIndex])
        throw std::runtime_error("Internal DB error: Searched key doesn't exist");
    return *this->keys[keyIndex];
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
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::add(TKey const &key, size_t descendantOffset) {
    if (this->full()) throw std::runtime_error("Unable to add new key, desc to full node");
    this->changed = true;
    std::pair<std::vector<TKey>, std::vector<size_t>> data = this->getEntries();
    auto insertPosition = std::upper_bound(data.first.begin(), data.first.end(), key) - data.first.begin();
    data.first.insert(data.first.begin() + insertPosition, key);
    data.second.insert(data.second.begin() + insertPosition + 1, descendantOffset);
    this->setEntries(data);
    return *this;
}


/**
 * Merges node with given node and given key and returns middle key,
 * assumes, that this is left node, and right node is given in param and also that parent is present and loaded
 * @tparam TKey
 * @tparam TValue
 * @tparam TDegree
 * @param node
 * @param key
 * @param value
 * @return middle key to put it in parent
 */
template<typename TKey, typename TValue, size_t TDegree>
TKey
InnerNode<TKey, TValue, TDegree>::compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const &key,
                                                                   TValue const &value, size_t nodeOffset) {
    if (node->nodeType() != NodeType::INNER)
        throw std::runtime_error("Internal DB error: compensation failed, bad neighbour node type");
    if (nodeOffset == 0)
        throw std::invalid_argument("Internal DB error: InnerNode compensation failed: nodeOffset cannot be 0");
    auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);
    auto parentKey = std::dynamic_pointer_cast<InnerNode>(this->parent)->getKeyAfterPointer(this->fileOffset);
    // get first node data
    auto[allKeys, allDescendants] = this->getEntries();
    auto[otherKeys, otherDescendants] = otherNode->getEntries();
    // add key from parent
    allKeys.push_back(parentKey);
    // add keys from second node
    allKeys.insert(allKeys.end(), otherKeys.begin(), otherKeys.end());
    // add new key
    auto insertPosition = allKeys.insert(std::upper_bound(allKeys.begin(), allKeys.end(), key), key) - allKeys.begin();
    allDescendants.insert(allDescendants.begin() + insertPosition, nodeOffset);
    // add descendants from other node
    allDescendants.insert(allDescendants.end(), otherDescendants.begin(), otherDescendants.end());

    auto mediumKeyIterator = allKeys.begin() + allKeys.size() / 2;
    auto mediumDescendantIterator = allDescendants.begin() + allDescendants.size() / 2;
    // split data among nodes
    this->setKeys(allKeys.begin(), mediumKeyIterator);
    this->setDescendants(allDescendants.begin(), mediumDescendantIterator);
    otherNode->setKeys(mediumKeyIterator + 1, allKeys.end());
    otherNode->setDescendants(mediumDescendantIterator, allDescendants.end());

    return *mediumKeyIterator;
}


#endif //SBD2_INNER_NODE_HH
