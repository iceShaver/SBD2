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
    using NodeOffset = typename Base::NodeOffset;
    using DescendantsCollection = std::array<std::optional<NodeOffset>, 2 * TDegree + 1>;
    using KeysCollection = std::array<std::optional<TKey>, 2 * TDegree>;
    using KeysVectorIterator = typename std::vector<TKey>::iterator;
    using DescendantsVectorIterator = typename std::vector<NodeOffset>::iterator;

    template<typename, typename, size_t, size_t> friend class BPlusTree;
public:
    InnerNode(NodeOffset fileOffset, File &file, std::shared_ptr<Base> const &parent = nullptr);
    ~InnerNode() override { this->unload(); };

    static constexpr auto BytesSize() { return sizeof(DescendantsCollection) + sizeof(KeysCollection); };
    auto getEntries() const -> std::pair<std::vector<TKey>, std::vector<NodeOffset>>;
    auto setEntries(std::pair<std::vector<TKey>, std::vector<NodeOffset>> const &entries) -> void;
    auto setKeys(KeysVectorIterator begI, KeysVectorIterator endI) -> void;
    auto setDescendants(DescendantsVectorIterator begI, DescendantsVectorIterator endI) -> void;
    auto compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const *key,
                                          TValue const *value,
                                          NodeOffset nodeOffset) -> TKey override;
    auto mergeWith(std::shared_ptr<Base> &node, TKey const *key = nullptr) -> void override;
    auto full() const -> bool override;
    auto add(TKey const &key, NodeOffset descendantOffset) -> void;
    auto setKeyBetweenPtrs(NodeOffset aPtr, NodeOffset bPtr, TKey const &key) -> void;
    auto removeKeyOffsetAfter(NodeOffset offset);
    auto getKeysRange();
    auto getDescendantsRange();
    auto getDescendantsOfKey(TKey const &key) -> std::pair<NodeOffset, NodeOffset>;
    auto getPrecedingKey(NodeOffset nodeOffset) -> std::optional<TKey>;
    auto swapKeys(TKey const &oldKey, TKey const &newKey) -> void;
    auto getNextDescendantOffset(NodeOffset offset) const -> std::optional<NodeOffset>;
    auto getPrevDescendantOffset(NodeOffset offset) const -> std::optional<NodeOffset>;
    auto getLastDescendantOffset() const -> NodeOffset;
    auto getAfterLastKeyIndex() const;
    auto getKeyBetweenPtrs(NodeOffset aPtr, NodeOffset bPtr) -> TKey;
    auto print(std::stringstream &ss) const -> std::stringstream & override;
    auto contains(TKey const &key) const -> bool override;
    auto fillKeysSize() const -> size_t override;
    auto degree() -> size_t override { return TDegree; };


private:
    auto getKeyIndexBetweenPtrs(NodeOffset aPtr, NodeOffset bPtr) -> NodeOffset;
    auto print(std::ostream &o) const -> std::ostream & override;
    auto deserialize(std::vector<Byte> const &bytes) -> void override;
    auto getData() -> std::vector<Byte> override;
    auto nodeType() const -> NodeType override { return NodeType::INNER; }
    auto bytesSize() const -> size_t override { return BytesSize(); }
    auto elementsSize() const -> size_t override { return ElementsSize(); }
    constexpr auto ElementsSize() const noexcept { return this->descendants.size() + this->keys.size() + 1; }


    DescendantsCollection descendants{};
    KeysCollection keys{};
};

template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getData() -> std::vector<Byte> {
    auto result = std::vector<Byte>();
    auto keysByteArray = (std::array<Byte, sizeof(this->keys)> *) this->keys.data();
    auto descendantsByteArray = (std::array<Byte, sizeof(this->descendants)> *) this->descendants.data();
    result.reserve(keysByteArray->size() + descendantsByteArray->size());
    std::copy(keysByteArray->begin(), keysByteArray->end(), std::back_inserter(result));
    std::copy(descendantsByteArray->begin(), descendantsByteArray->end(), std::back_inserter(result));
    return std::move(result);
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::deserialize(std::vector<Byte> const &bytes) -> void {
    this->changed = true;
    auto descendantsBytePtr = (std::array<Byte, sizeof(this->descendants)> *) this->descendants.data();
    auto keysBytePtr = (std::array<Byte, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin(), keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + keysBytePtr->size(), descendantsBytePtr->size(), descendantsBytePtr->begin());
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
    if (i != 0)
        o << "D:" << descendants[i];
    o << '}'; // TODO: changeit after the fix
    return o;
}


template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree>::InnerNode(NodeOffset fileOffset, File &file,
                                            std::shared_ptr<InnerNode::Base> const &parent) : Base(fileOffset,
                                                                                                   file,
                                                                                                   parent) {}


template<typename TKey, typename TValue, size_t TDegree>
size_t
InnerNode<TKey, TValue, TDegree>::fillKeysSize() const {
    return static_cast<size_t>(std::count_if(this->keys.begin(), this->keys.end(),
                                             [](auto x) { return x != std::nullopt; }));
}


template<typename TKey, typename TValue, size_t TDegree>
bool
InnerNode<TKey, TValue, TDegree>::full() const {
    return std::find(this->descendants.begin(), this->descendants.end(), std::nullopt) == this->descendants.end();
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getEntries() const -> std::pair<std::vector<TKey>, std::vector<NodeOffset>> {
    auto result = std::pair<std::vector<TKey>, std::vector<NodeOffset>>();
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
void
InnerNode<TKey, TValue, TDegree>::setEntries(std::pair<std::vector<TKey>, std::vector<NodeOffset>> const &entries) {
    this->changed = true;
    this->keys = KeysCollection();
    this->descendants = DescendantsCollection();
    std::copy(entries.first.begin(), entries.first.end(), this->keys.begin());
    std::copy(entries.second.begin(), entries.second.end(), this->descendants.begin());
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::setKeys(KeysVectorIterator begI, KeysVectorIterator endI) {
    this->changed = true;
    this->keys = KeysCollection();
    std::copy(begI, endI, this->keys.begin());
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::setDescendants(DescendantsVectorIterator begI, DescendantsVectorIterator endI) {
    this->changed = true;
    this->descendants = DescendantsCollection();
    std::copy(begI, endI, this->descendants.begin());
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::add(TKey const &key, NodeOffset descendantOffset) {
    if (this->full()) throw std::runtime_error("Unable to add new key, desc to full node");
    this->changed = true;
    auto data = this->getEntries();
    auto insertPosition = std::upper_bound(data.first.begin(), data.first.end(), key) - data.first.begin();
    data.first.insert(data.first.begin() + insertPosition, key);
    data.second.insert(data.second.begin() + insertPosition + 1, descendantOffset);
    this->setEntries(data);
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
InnerNode<TKey, TValue, TDegree>::compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const *const key,
                                                                   TValue const *const value,
                                                                   NodeOffset nodeOffset) {
    if (node->nodeType() != NodeType::INNER)
        throw std::runtime_error("Internal DB error: compensation failed, bad neighbour node type");
    /*if (nodeOffset == 0)
        throw std::invalid_argument("Internal DB error: InnerNode compensation failed: nodeOffset cannot be 0");*/

    auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);

    std::vector<TKey> allKeys;
    std::vector<NodeOffset> allDescendants;

    // get nodes data
    auto[aKeys, aDescendants] = this->getEntries();
    auto[bKeys, bDescendants] = otherNode->getEntries();

    // add data from first node
    std::move(aKeys.begin(), aKeys.end(), std::back_inserter(allKeys));
    std::move(aDescendants.begin(), aDescendants.end(), std::back_inserter(allDescendants));

    // not loaded node means it is newly created -> means we are performing split operation
    // (where we don't add parent key and data from second node, because it's empty)
    if (otherNode->loaded) {
        if (this->parent == nullptr)
            throw std::invalid_argument("Internal DB error: InnerNode compensation failed: parent node is NULL");
        auto parentKey = std::dynamic_pointer_cast<InnerNode>(this->parent)->getKeyBetweenPtrs(this->fileOffset,
                                                                                               node->fileOffset);
        allKeys.push_back(parentKey);
        std::move(bKeys.begin(), bKeys.end(), std::back_inserter(allKeys));
        std::move(bDescendants.begin(), bDescendants.end(), std::back_inserter(allDescendants));
    }

    if (key != nullptr) {
        // add new key
        auto insertIterator = allKeys.insert(std::upper_bound(allKeys.begin(), allKeys.end(), *key), *key);
        auto insertPosition = insertIterator - allKeys.begin(); // DO NOT put that in one line with above

        // add new descendant
        allDescendants.insert(allDescendants.begin() + insertPosition + 1, nodeOffset);
    }

    // get middleKey and middleDescendant
    auto middleKeyIterator = allKeys.begin() + (allKeys.size()) / 2;
    auto middleDescendantIterator = allDescendants.begin() + (allDescendants.size() - 1) / 2;

    // if size of descendants and keys is bad -> something went wrong
    if (allDescendants.size() != allKeys.size() + 1)
        throw std::runtime_error("Internal DB error: descendants and keys numbers are incorrect after compensation");

    // split data among nodes
    this->setKeys(allKeys.begin(), middleKeyIterator);
    otherNode->setKeys(middleKeyIterator + 1, allKeys.end());
    this->setDescendants(allDescendants.begin(), middleDescendantIterator + 1);
    otherNode->setDescendants(middleDescendantIterator + 1, allDescendants.end());

    return *middleKeyIterator;
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::mergeWith(std::shared_ptr<Base> &node, TKey const *const key) {
    if (node->nodeType() != NodeType::INNER)
        throw std::runtime_error("Internal DB error: merge failed, bad neighbour node type");
    auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);
    auto[firstKeysBegin, firstKeysEnd] = this->getKeysRange();
    auto[secondKeysBegin, secondKeysEnd] = otherNode->getKeysRange();
    auto[firstDescendantsBegin, firstDescendantsEnd] = this->getDescendantsRange();
    auto[secondDescendantsBegin, secondDescendantsEnd] = otherNode->getDescendantsRange();
    if (key) *firstKeysEnd = *key;
    std::move(secondKeysBegin, secondKeysEnd, firstKeysEnd + 1);
    std::move(secondDescendantsBegin, secondDescendantsEnd, firstDescendantsEnd);
    this->markChanged();
    otherNode->remove();

}

template<typename TKey, typename TValue, size_t TDegree>
std::stringstream &
InnerNode<TKey, TValue, TDegree>::print(std::stringstream &ss) const {
    ss << "node" << this->fileOffset << "[xlabel=<<font color='#aaffaa'>" << this->fileOffset << "</font>> label=\"";
    auto[keys, descendants] = this->getEntries();
    int i;
    for (i = 0; i < keys.size(); ++i) {
        ss << "<f" << i << ">" << /*descendants[i]*/"" << "|" << keys[i] << '|';
    }
    ss << "<f" << i << '>' << /*descendants[i]*/"" << "\"];\n";
    for (int i = 0; i < descendants.size(); ++i) {
        ss << "\"node" << this->fileOffset << "\":" << 'f' << i << " -> \"node" << descendants[i] << "\"\n";
    }
    return ss;
}


template<typename TKey, typename TValue, size_t TDegree>
bool InnerNode<TKey, TValue, TDegree>::contains(TKey const &key) const {
    for (auto &element : keys) {
        if (!element) return false;
        if (*element == key) return true;
    }
    return false;
}


template<typename TKey, typename TValue, size_t TDegree>
TKey
InnerNode<TKey, TValue, TDegree>::getKeyBetweenPtrs(NodeOffset aPtr, NodeOffset bPtr) {
    auto result = keys[getKeyIndexBetweenPtrs(aPtr, bPtr)];
    if (!result)
        throw std::runtime_error("Key between ptrs is nullopt: " + std::to_string(aPtr) + ' ' + std::to_string(bPtr));
    return *result;
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getKeyIndexBetweenPtrs(NodeOffset aPtr, NodeOffset bPtr) -> NodeOffset {
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
void
InnerNode<TKey, TValue, TDegree>::setKeyBetweenPtrs(NodeOffset aPtr, NodeOffset bPtr, TKey const &key) {
    auto index = getKeyIndexBetweenPtrs(aPtr, bPtr);
    this->changed = true;
    keys[index] = key;
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getNextDescendantOffset(NodeOffset offset) const -> std::optional<NodeOffset> {
    for (int i = 0; i < this->descendants.size(); ++i) {
        if (this->descendants[i] == std::nullopt)
            return std::nullopt;
        if (this->descendants[i] == offset) {
            if (i < this->descendants.size() - 1 && this->descendants[i + 1]) {
                return this->descendants[i + 1];
            }
            return std::nullopt;
        }
    }
    throw std::runtime_error("Given descendant offset doesn't exist");
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getPrevDescendantOffset(NodeOffset offset) const -> std::optional<NodeOffset> {
    for (int i = 0; i < this->descendants.size(); ++i) {
        if (this->descendants[i] == std::nullopt) {
            return std::nullopt;
        }
        if (this->descendants[i] == offset) {
            if (i == 0) {
                return std::nullopt;
            }
            return this->descendants[i - 1];
        }
    }
    throw std::runtime_error("Given descendant offset doesn't exist");
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getLastDescendantOffset() const -> NodeOffset {
    return **std::find_if(descendants.rbegin(), descendants.rend(), [](auto x) { return x != std::nullopt; });
}


template<typename TKey, typename TValue, size_t TDegree>
void InnerNode<TKey, TValue, TDegree>::swapKeys(TKey const &oldKey, TKey const &newKey) {

    auto it = std::find(keys.begin(), keys.end(), oldKey);
    if (it == keys.end()) {
        throw std::runtime_error("Internal DB error: InnerNode->swapKeys: unable to find given key");
    }
    *it = newKey;
    this->changed = true;
}

template<typename TKey, typename TValue, size_t TDegree>
auto InnerNode<TKey, TValue, TDegree>::getAfterLastKeyIndex() const {
    return std::distance(std::find_if(keys.rbegin(), keys.rend(), [](auto x) { return x != std::nullopt; }),
                         keys.rend());
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getKeysRange() {
    auto begin = keys.begin();
    auto end = std::find_if(keys.rbegin(), keys.rend(), [](auto x) { return x != std::nullopt; }).base();
    return std::pair(begin, end);
}


template<typename TKey, typename TValue, size_t TDegree>
auto InnerNode<TKey, TValue, TDegree>::getDescendantsRange() {
    auto begin = descendants.begin();
    auto end = std::find_if(descendants.rbegin(), descendants.rend(), [](auto x) { return x != std::nullopt; }).base();
    return std::pair(begin, end);
}


template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::removeKeyOffsetAfter(NodeOffset offset) {
    int i = 0;
    for (auto &o:descendants) {
        if (o == std::nullopt) {
            throw std::runtime_error("Internal DB error: innerNode: removeKeyOffsetAfter: given offset not found");
        }
        if (o == offset) break;
        ++i;
    }
    i++; // now i is index of offset to delete
    // removing offset
    auto lastDesc = std::move(descendants.begin() + i + 1, descendants.end(), descendants.begin() + i);
    std::fill(lastDesc, descendants.end(), std::nullopt);
    // removing key
    auto lastKey = std::move(keys.begin() + i, keys.end(), keys.begin() + i - 1);
    std::fill(lastKey, keys.end(), std::nullopt);
    this->changed = true;
    if (this->fillKeysSize() < TDegree)
        return NodeState::TOO_SMALL;
    return NodeState::OK;
}

template<typename TKey, typename TValue, size_t TDegree>
auto
InnerNode<TKey, TValue, TDegree>::getDescendantsOfKey(TKey const &key) -> std::pair<NodeOffset, NodeOffset> {
    auto[keysBegin, keysEnd] = getKeysRange();
    auto i = std::lower_bound(keysBegin, keysEnd, key) - keysBegin;
    auto l = *descendants[i];
    auto p = *descendants[i + 1];
    return std::pair(l, p);

}
template<typename TKey, typename TValue, size_t TDegree>
auto InnerNode<TKey, TValue, TDegree>::getPrecedingKey(InnerNode::NodeOffset nodeOffset) -> std::optional<TKey> {
    auto [b, e] = getDescendantsRange();
    auto result = std::find(b, e, nodeOffset);
    if(result == e) return std::nullopt;
    return keys[result - b];
}


#endif //SBD2_INNER_NODE_HH
