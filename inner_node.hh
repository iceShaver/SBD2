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
    TKey compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const &key, TValue const &value,
                                          size_t nodeOffset) override;
    bool full() const override;
    InnerNode &add(TKey const &key, size_t descendantOffset);
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

    auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);

    std::vector<TKey> allKeys;
    std::vector<size_t> allDescendants;

    // get nodes data
    auto[aKeys, aDescendants] = this->getEntries();
    auto[bKeys, bDescendants] = otherNode->getEntries();


    // determine order (which node is left, right) (contains smaller keys), if otherNode is newly created, then it is right
    bool order = (bKeys.size() != 0) ? aKeys[0] < bKeys[0] : true;
    auto keys = order ? std::pair(std::move(aKeys), std::move(bKeys))
                      : std::pair(std::move(bKeys), std::move(aKeys));
    auto descendants = order ? std::pair(std::move(aDescendants), std::move(bDescendants))
                             : std::pair(std::move(bDescendants), std::move(aDescendants));
    auto nodes = order ? std::pair(this, otherNode.get())
                       : std::pair(otherNode.get(), this);

    // add data from first node
    std::move(keys.first.begin(), keys.first.end(), std::back_inserter(allKeys));
    std::move(descendants.first.begin(), descendants.first.end(), std::back_inserter(allDescendants));

    // not loaded node means it is newly created -> means we are performing split operation
    // (where we don't add parent key and data from second node, because it's empty)
    if (otherNode->loaded) {
        if (this->parent == nullptr)
            throw std::invalid_argument("Internal DB error: InnerNode compensation failed: parent node is NULL");
        auto parentKey = std::dynamic_pointer_cast<InnerNode>(this->parent)->getKeyBetweenPtrs(this->fileOffset,
                                                                                               node->fileOffset);
        allKeys.push_back(parentKey);
        std::move(keys.second.begin(), keys.second.end(), std::back_inserter(allKeys));
        std::move(descendants.second.begin(), descendants.second.end(), std::back_inserter(allDescendants));
    }

    // add new key
    auto insertIterator = allKeys.insert(std::upper_bound(allKeys.begin(), allKeys.end(), key), key);
    auto insertPosition = insertIterator - allKeys.begin(); // DO NOT put that in one line with above

    // add new descendant
    allDescendants.insert(allDescendants.begin() + insertPosition + 1, nodeOffset);

    // get middleKey and middleDescendant
    auto middleKeyIterator = allKeys.begin() + (allKeys.size() - 1) / 2;
    auto middleDescendantIterator = allDescendants.begin() + (allDescendants.size() - 1) / 2;

    // if size of descendants and keys is bad -> something went wrong
    if (allDescendants.size() != allKeys.size() + 1)
        throw std::runtime_error("Internal DB error: descendants and keys numbers are incorrect after compensation");

    // split data among nodes
    nodes.first->setKeys(allKeys.begin(), middleKeyIterator);
    nodes.second->setKeys(middleKeyIterator + 1, allKeys.end());
    nodes.first->setDescendants(allDescendants.begin(), middleDescendantIterator + 1);
    nodes.second->setDescendants(middleDescendantIterator + 1, allDescendants.end());

    return *middleKeyIterator;
}


template<typename TKey, typename TValue, size_t TDegree>
std::stringstream &
InnerNode<TKey, TValue, TDegree>::print(std::stringstream &ss) const {
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
size_t
InnerNode<TKey, TValue, TDegree>::getKeyIndexBetweenPtrs(size_t aPtr, size_t bPtr) {
    for (auto i = 0u; i < descendants.size(); ++i) {
        auto desc = descendants[i];
        if (i == descendants.size() - 1) throw std::runtime_error("Internal DB error: searched key not found");
        if (!desc) throw std::runtime_error("Internal DB error: searched key not found");
        if (*desc == aPtr || *desc == bPtr) {
            return i;
        }
    }
    throw std::runtime_error("Internal DB error: searched key not found");
}


template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree> &
InnerNode<TKey, TValue, TDegree>::setKeyBetweenPtrs(size_t aPtr, size_t bPtr, TKey const &key) {
    auto index = getKeyIndexBetweenPtrs(aPtr, bPtr);
    this->changed = true;
    keys[index] = key;
    return *this;
}


#endif //SBD2_INNER_NODE_HH
