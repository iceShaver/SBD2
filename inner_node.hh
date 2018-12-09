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
    //std::optional<TKey> getKeyAfterPointer(std::optional<size_t> aPtr);
    TKey compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const &key, TValue const &value,
                                          size_t nodeOffset) override;
    bool full() const override;
    InnerNode &add(TKey const &key, size_t descendantOffset);
    InnerNode &changeKey(size_t aPtr, TKey const &key);
    InnerNode &setKeyBetweenPtrs(size_t aPtr, size_t bPtr, TKey const &key);
    TKey getKeyBetweenPtrs(size_t aPtr, size_t bPtr);
    std::stringstream &print(std::stringstream &ss) const override;


    DescendantsCollection descendants;
    KeysCollection keys;

private:
    size_t getKeyIndexBetweenPtrs(size_t aPtr, size_t bPtr);
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
    int i;
    for (i = 0; i < keys.size(); ++i) {
        o << "D:" << descendants[i] << ' ' << "K:" << keys[i] << ' ';
    }
    return o << "D:" << descendants[i] << '}'; // TODO: changeit after the fix
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
    /*  if(this->loaded == false)
          throw std::runtime_error("Node not loaded: " + std::to_string(this->fileOffset));*/
    return std::find(this->descendants.begin(), this->descendants.end(), std::nullopt) == this->descendants.end();
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


template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::setKeyBetweenPtrs(size_t aPtr, size_t bPtr, TKey const &key) {
    auto index = getKeyIndexBetweenPtrs(aPtr, bPtr);
    this->changed = true;
    keys[index] = key;
    return *this;
}

/*
// TODO: this is nonsens
template<typename TKey, typename TValue, size_t TDegree>
std::optional<TKey>
InnerNode<TKey, TValue, TDegree>::getKeyAfterPointer(std::optional<size_t> aPtr) {
    auto foundIter = std::find(this->descendants.begin(), this->descendants.end(), aPtr);
    if (foundIter == this->descendants.end())
        throw std::runtime_error("Internal DB error: couldn't find given ptr in parent!");
    auto keyIndex = foundIter - this->descendants.begin();
    if (keyIndex >= this->keys.size())
        return std::nullopt;
//        throw std::runtime_error("Internal DB error: Searched key doesn't exist");
    return *this->keys[keyIndex];
}*/


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
    bool isRoot = false;
    if (node->nodeType() != NodeType::INNER)
        throw std::runtime_error("Internal DB error: compensation failed, bad neighbour node type");
    if (nodeOffset == 0)
        throw std::invalid_argument("Internal DB error: InnerNode compensation failed: nodeOffset cannot be 0");
    if (this->parent == nullptr)
        //throw std::invalid_argument("Internal DB error: InnerNode compensation failed: parent node is NULL");
        isRoot = true;

    if (isRoot) {
        auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);
        auto[keys, descendants] = this->getEntries();
        auto insertIterator = keys.insert(std::upper_bound(keys.begin(), keys.end(), key), key);
        auto insertPosition = insertIterator - keys.begin(); // DO NOT put that in one line with above
        descendants.insert(descendants.begin() + insertPosition + 1, nodeOffset);
        auto mediumKeyIterator = keys.begin() + keys.size() / 2;
        auto mediumDescendantIterator = descendants.begin() + descendants.size() / 2;
        if (descendants.size() != keys.size() + 1)
            throw std::runtime_error(
                    "Internal DB error: descendants and keys numbers are incorrect after compensation");
        this->setKeys(keys.begin(), mediumKeyIterator);
        otherNode->setKeys(mediumKeyIterator + 1, keys.end());
        this->setDescendants(descendants.begin(), mediumDescendantIterator);
        otherNode->setDescendants(mediumDescendantIterator, descendants.end());
        return *mediumKeyIterator;
    }

    auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);
    // get first node data
    std::vector<TKey> allKeys;
    std::vector<size_t> allDescendants;


    auto[aKeys, aDescendants] = this->getEntries();
    auto[bKeys, bDescendants] = otherNode->getEntries();
    TKey parentKey;
    if (otherNode->loaded)
        parentKey = std::dynamic_pointer_cast<InnerNode>(this->parent)->getKeyBetweenPtrs(this->fileOffset,
                                                                                          node->fileOffset);
    // determine order (which node is left, right) (contains smaller keys), if otherNode is newly created, then it is second
    bool order = (bKeys.size() != 0) ? aKeys[0] < bKeys[0] : true;
    auto keys = order ? std::pair(std::move(aKeys), std::move(bKeys))
                      : std::pair(std::move(bKeys), std::move(aKeys));
    auto descendants = order ? std::pair(std::move(aDescendants), std::move(bDescendants))
                             : std::pair(std::move(bDescendants), std::move(aDescendants));

    // put all keys and descendants together
    std::move(keys.first.begin(), keys.first.end(), std::back_inserter(allKeys));
    std::move(descendants.first.begin(), descendants.first.end(), std::back_inserter(allDescendants));
    // not loaded node means it is newly created -> means we are performing split operation
    if (otherNode->loaded)
        allKeys.push_back(parentKey);
    std::move(keys.second.begin(), keys.second.end(), std::back_inserter(allKeys));
    std::move(descendants.second.begin(), descendants.second.end(), std::back_inserter(allDescendants));

    // add new key
    auto insertIterator = allKeys.insert(std::upper_bound(allKeys.begin(), allKeys.end(), key), key);
    auto insertPosition = insertIterator - allKeys.begin(); // DO NOT put that in one line with above

    // add new descendant
    //auto beginPos = (insertPosition < allDescendants.size()) ? allDescendants.begin() + insertPosition + 1 :
    //               allDescendants.begin() + insertPosition;
    allDescendants.insert(allDescendants.begin() + insertPosition + 1, nodeOffset);

    auto mediumKeyIterator = allKeys.begin() + (allKeys.size() - 1) / 2;
    auto mediumDescendantIterator = allDescendants.begin() + (allDescendants.size() - 1) / 2;

    // care situation where key is duplicated from parent
    allKeys.resize(allDescendants.size() - 1);
    // if (allDescendants.size() != allKeys.size() + 1)
    //   throw std::runtime_error("Internal DB error: descendants and keys numbers are incorrect after compensation");
    //that shouldn't be necessary
    //allKeys.resize(allDescendants.size() - 1); // remove surplus key, if no descendant after it
    // split data among nodes
    if (order) {
        this->setKeys(allKeys.begin(), mediumKeyIterator);
        otherNode->setKeys(mediumKeyIterator + 1, allKeys.end());
        this->setDescendants(allDescendants.begin(), mediumDescendantIterator + 1); // TODO: or +1
        otherNode->setDescendants(mediumDescendantIterator + 1, allDescendants.end()); // TODO: or +1
    } else {
        otherNode->setKeys(allKeys.begin(), mediumKeyIterator);
        this->setKeys(mediumKeyIterator + 1, allKeys.end());
        otherNode->setDescendants(allDescendants.begin(), mediumDescendantIterator + 1); // TODO: or +1
        this->setDescendants(mediumDescendantIterator + 1, allDescendants.end()); // TODO: or +1
    }

    return *mediumKeyIterator;
}
template<typename TKey, typename TValue, size_t TDegree>
std::stringstream &InnerNode<TKey, TValue, TDegree>::print(std::stringstream &ss) const {
    ss << "node" << this->fileOffset << "[label=\"";
    auto[keys, descendants] = this->getEntries();
    int i;
    for (i = 0; i < keys.size(); ++i) {
        ss << "<f" << i << ">" << /*descendants[i]*/"*" << "|" << keys[i] << '|';
    }
    ss << "<f" << i << '>' << /*descendants[i]*/"*" << "\"];\n"; // TODO: changeit after the fix
    for (int i = 0; i < descendants.size(); ++i) {
        ss << "\"node" << this->fileOffset << "\":" << 'f' << i << " -> \"node" << descendants[i] << "\"\n";
    }
    return ss;
}


template<typename TKey, typename TValue, size_t TDegree>
TKey
InnerNode<TKey, TValue, TDegree>::getKeyBetweenPtrs(size_t aPtr, size_t bPtr) {
    auto result = keys[getKeyIndexBetweenPtrs(aPtr, bPtr)];
    if (!result)
        throw std::runtime_error("Key between ptrs is nullopt: " + std::to_string(aPtr) + ' ' + std::to_string(bPtr));
    return *result;
}


template<typename TKey, typename TValue, size_t TDegree>
size_t InnerNode<TKey, TValue, TDegree>::getKeyIndexBetweenPtrs(size_t aPtr, size_t bPtr) {
    auto keyIndex = 0;
    for (int i = 0; i < descendants.size(); ++i) {
        auto desc = descendants[i];
        if (i == descendants.size() - 1) throw std::runtime_error("Internal DB error: searched key not found");
        if (!desc) throw std::runtime_error("Internal DB error: searched key not found");
        if (*desc == aPtr || *desc == bPtr) {
            return keyIndex = i;
            if (!descendants[i + 1] || (descendants[i + 1] != aPtr && descendants[i + 1] != bPtr))
                throw std::runtime_error("Internal DB error: bad second ptr in setKeyBetweenPtrs in " +
                                         std::to_string(this->fileOffset) + ": " + std::to_string(aPtr) + " " +
                                         std::to_string(bPtr));
            break;
        }
    }
    return keyIndex;
}


#endif //SBD2_INNER_NODE_HH
