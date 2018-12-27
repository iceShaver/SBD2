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
    InnerNode(size_t fileOffset, File &file, std::shared_ptr<Base> const &parent = nullptr);
    ~InnerNode() override { this->unload(); };

    static constexpr auto BytesSize() { return sizeof(DescendantsCollection) + sizeof(KeysCollection); };
    auto getEntries() const -> std::pair<std::vector<TKey>, std::vector<size_t>>;
    auto setEntries(std::pair<std::vector<TKey>, std::vector<size_t>> const &entries) -> void;
    auto setKeys(typename std::vector<TKey>::iterator begIt, typename std::vector<TKey>::iterator endIt) -> void;
    auto setDescendants(typename std::vector<size_t>::iterator begIt,
                        typename std::vector<size_t>::iterator endIt) -> void;

    auto compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const *key,
                                          TValue const *value,
                                          size_t nodeOffset) -> TKey override;
    auto mergeWith(std::shared_ptr<Base> &node, TKey const *key = nullptr) -> void override;
    auto full() const -> bool override;
    auto add(TKey const &key, size_t descendantOffset) -> void;
    auto setKeyBetweenPtrs(size_t aPtr, size_t bPtr, TKey const &key) -> void;
    auto removeKeyOffsetAfter(size_t offset);
    auto getKeysRange();
    auto getDescendantsRange();
    auto swapKeys(TKey const &oldKey, TKey const &newKey) -> void;
    auto getNextDescendantOffset(size_t offset) const -> std::optional<size_t>;
    auto getPrevDescendantOffset(size_t offset) const -> std::optional<size_t>;
    auto getLastDescendantOffset() const -> size_t;
    auto getAfterLastKeyIndex() const;
    auto getKeyBetweenPtrs(size_t aPtr, size_t bPtr) -> TKey;
    auto print(std::stringstream &ss) const -> std::stringstream & override;
    auto contains(TKey const &key) const -> bool override;
    auto fillKeysSize() const -> size_t override;
    auto degree() -> size_t override { return TDegree; };

    DescendantsCollection descendants{};
    KeysCollection keys{};

private:
    auto getKeyIndexBetweenPtrs(size_t aPtr, size_t bPtr) -> size_t;
    auto print(std::ostream &o) const -> std::ostream & override;
    auto deserialize(std::vector<char> const &bytes) -> Base & override;
    auto getData() -> std::vector<uint8_t> override;
    auto nodeType() const -> NodeType override { return NodeType::INNER; }
    auto bytesSize() const -> size_t override { return BytesSize(); }
    auto elementsSize() const -> size_t override { return ElementsSize(); }
    constexpr auto ElementsSize() const noexcept { return this->descendants.size() + this->keys.size() + 1; }
};

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
Node<TKey, TValue> &
InnerNode<TKey, TValue, TDegree>::deserialize(std::vector<char> const &bytes) {
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
    if (i != 0)
        o << "D:" << descendants[i];
    o << '}'; // TODO: changeit after the fix
    return o;
}


template<typename TKey, typename TValue, size_t TDegree>
InnerNode<TKey, TValue, TDegree>::InnerNode(size_t fileOffset, File &file,
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
void
InnerNode<TKey, TValue, TDegree>::setEntries(std::pair<std::vector<TKey>, std::vector<size_t>> const &entries) {
    this->changed = true;
    this->keys = KeysCollection();
    this->descendants = DescendantsCollection();
    std::copy(entries.first.begin(), entries.first.end(), this->keys.begin());
    std::copy(entries.second.begin(), entries.second.end(), this->descendants.begin());
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::setKeys(typename std::vector<TKey>::iterator begIt,
                                          typename std::vector<TKey>::iterator endIt) {
    this->changed = true;
    this->keys = KeysCollection();
    std::copy(begIt, endIt, this->keys.begin());
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::setDescendants(typename std::vector<size_t>::iterator begIt,
                                                 typename std::vector<size_t>::iterator endIt) {
    this->changed = true;
    this->descendants = DescendantsCollection();
    std::copy(begIt, endIt, this->descendants.begin());
}


template<typename TKey, typename TValue, size_t TDegree>
void
InnerNode<TKey, TValue, TDegree>::add(TKey const &key, size_t descendantOffset) {
    if (this->full()) throw std::runtime_error("Unable to add new key, desc to full node");
    this->changed = true;
    std::pair<std::vector<TKey>, std::vector<size_t>> data = this->getEntries();
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
                                                                   size_t nodeOffset) {
    if (node->nodeType() != NodeType::INNER)
        throw std::runtime_error("Internal DB error: compensation failed, bad neighbour node type");
    /*if (nodeOffset == 0)
        throw std::invalid_argument("Internal DB error: InnerNode compensation failed: nodeOffset cannot be 0");*/

    auto otherNode = std::dynamic_pointer_cast<InnerNode>(node);

    std::vector<TKey> allKeys;
    std::vector<size_t> allDescendants;

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
    auto middleKeyIterator = allKeys.begin() + (allKeys.size() - 1) / 2;
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
void
InnerNode<TKey, TValue, TDegree>::setKeyBetweenPtrs(size_t aPtr, size_t bPtr, TKey const &key) {
    auto index = getKeyIndexBetweenPtrs(aPtr, bPtr);
    this->changed = true;
    keys[index] = key;
}


template<typename TKey, typename TValue, size_t TDegree>
std::optional<size_t>
InnerNode<TKey, TValue, TDegree>::getNextDescendantOffset(size_t offset) const {
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
std::optional<size_t>
InnerNode<TKey, TValue, TDegree>::getPrevDescendantOffset(size_t offset) const {
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
size_t
InnerNode<TKey, TValue, TDegree>::getLastDescendantOffset() const {
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
InnerNode<TKey, TValue, TDegree>::removeKeyOffsetAfter(size_t offset) {
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


#endif //SBD2_INNER_NODE_HH
