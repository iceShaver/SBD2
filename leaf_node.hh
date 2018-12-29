//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_LEAF_NODE_HH
#define SBD2_LEAF_NODE_HH

#include <optional>
#include <array>
#include "node.hh"

template<typename TKey, typename TValue, size_t TDegree>
class LeafNode final : public Node<TKey, TValue> {
    using Base = Node<TKey, TValue>;
    using KeysCollection = std::array<std::optional<TKey>, 2 * TDegree>;
    using ValuesCollection =  std::array<std::optional<TValue>, 2 * TDegree>;
    using KeysIterator = typename KeysCollection::iterator;
    using ValuesIterator = typename ValuesCollection::iterator;
    using KeysValuesIterator = typename std::vector<std::pair<TKey, TValue>>::iterator;
    using KeysReverseIterator = typename KeysCollection::reverse_iterator;
    using ValuesReverseIterator = typename ValuesCollection::reverse_iterator;
    using KeysRange = std::pair<KeysIterator, KeysIterator>;
    using ValuesRange = std::pair<ValuesIterator, ValuesIterator>;
    using KeysReverseRange = std::pair<KeysReverseIterator, KeysReverseIterator>;
    using ValuesReverseRange = std::pair<ValuesReverseIterator, ValuesReverseIterator>;

    template<typename, typename, size_t, size_t> friend class BPlusTree;
public:
    LeafNode(size_t fileOffset, File &file, std::shared_ptr<Base> parent = nullptr) : Base(fileOffset, file, parent) {}
    ~LeafNode() override { this->unload(); }


    static auto BytesSize() { return sizeof(KeysCollection) + sizeof(ValuesCollection); }
    auto insert(TKey const &key, TValue const &value) -> void;
    auto readRecord(TKey const &key) const -> std::optional<TValue>;
    auto updateRecord(TKey const &key, TValue const &value) -> void;
    auto deleteRecord(TKey const &) -> NodeState;
    auto full() const -> bool override { return std::none_of(keys.begin(), keys.end(), [](auto x) { return !x; }); }
    auto contains(TKey const &key) const -> bool override;
    auto compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const *key,
                                          TValue const *value,
                                          size_t nodeOffset) -> TKey override;

    auto mergeWith(std::shared_ptr<Base> &other, TKey const *) -> void override;
    auto getRecords() const -> std::vector<std::pair<TKey, TValue>>;
    auto getLastRecordIndex() const -> long;
    auto getKeysRange() -> KeysRange;
    auto getKeysRangeReverse() -> KeysReverseRange;
    auto getValuesRange() -> ValuesRange;
    auto getValuesRangeReverse() -> ValuesReverseRange;
    auto getLastKey() const { return *std::find_if(keys.rbegin(), keys.rend(), [](auto x) { return x; }); }
    auto setRecords(KeysValuesIterator it1, KeysValuesIterator it2) -> void;
    auto print(std::stringstream &ss) const -> std::stringstream & override;
    auto fillKeysSize() const -> size_t override;
    virtual size_t degree() { return TDegree; }


private:
    auto print(std::ostream &o) const -> std::ostream & override;
    auto deserialize(std::vector<Byte> const &bytes) -> void override;
    auto getData() -> std::vector<Byte> override;
    auto elementsSize() const -> size_t override { return ElementsSize(); }
    auto bytesSize() const -> size_t override { return BytesSize(); }
    auto nodeType() const -> NodeType { return NodeType::LEAF; }
    constexpr auto ElementsSize() const noexcept { return this->keys.size() + this->values.size() + 1; }


    KeysCollection keys{};
    ValuesCollection values{};
public:

};


template<typename TKey, typename TValue, size_t TDegree>
auto
LeafNode<TKey, TValue, TDegree>::getData() -> std::vector<Byte>{
    auto result = std::vector<Byte>();
    auto keysByteArray = (std::array<Byte, sizeof(this->keys)> *) this->keys.data();
    auto valuesByteArray = (std::array<Byte, sizeof(this->values)> *) (this->values.data());
    result.reserve(keysByteArray->size() + valuesByteArray->size());
    std::copy(keysByteArray->begin(), keysByteArray->end(), std::back_inserter(result));
    std::copy(valuesByteArray->begin(), valuesByteArray->end(), std::back_inserter(result));
    return std::move(result);
}


template<typename TKey, typename TValue, size_t TDegree>
auto
LeafNode<TKey, TValue, TDegree>::deserialize(std::vector<Byte> const &bytes) -> void{
    this->changed = true;
    auto valuesBytePtr = (std::array<Byte, sizeof(this->values)> *) this->values.data();
    auto keysBytePtr = (std::array<Byte, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin(), keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + keysBytePtr->size(), valuesBytePtr->size(), valuesBytePtr->begin());
}


template<typename TKey, typename TValue, size_t TDegree>
std::ostream &
LeafNode<TKey, TValue, TDegree>::print(std::ostream &o) const {
    o << "LNode: " << this->fileOffset << " { ";
    for (auto&[k, v] : this->getRecords())
        o << "K:" << k << ' ';// << "V:" << v << ' ';
    return o << '}';
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
LeafNode<TKey, TValue, TDegree>::fillKeysSize() const {
    return static_cast<size_t>(std::count_if(keys.begin(), keys.end(), [](auto x) { return x; }));
}


template<typename TKey, typename TValue, size_t TDegree>
void
LeafNode<TKey, TValue, TDegree>::insert(TKey const &key, TValue const &value) {
    if (this->full()) throw std::runtime_error("Tried to add element to full node");
    this->markChanged();
    auto[keysBeginReverse, keysEndReverse] = getKeysRangeReverse();
    auto[keysBegin, keysEnd] = getKeysRange();
    auto[valuesBeginReverse, valuesEndReverse] = getValuesRangeReverse();
    auto i = std::upper_bound(keysBegin, keysEnd, key) - keysBegin;
    std::move(keysBeginReverse, keysEndReverse - i, keysBeginReverse - 1);
    keys[i] = key;
    std::move(valuesBeginReverse, valuesEndReverse - i, valuesBeginReverse - 1);
    values[i] = value;
}


template<typename TKey, typename TValue, size_t TDegree>
std::optional<TValue>
LeafNode<TKey, TValue, TDegree>::readRecord(TKey const &key) const {
    for (int i = 0; i < keys.size(); ++i) {
        if (keys[i] == std::nullopt) break;
        if (keys[i] == key) return values[i];
    }
    return std::nullopt;
}


template<typename TKey, typename TValue, size_t TDegree>
void
LeafNode<TKey, TValue, TDegree>::updateRecord(TKey const &key, TValue const &value) {
    for (int i = 0; i < keys.size(); ++i) {
        if (keys[i] == std::nullopt) break;
        if (keys[i] == key) {
            values[i]->update(value);
            this->markChanged();
            return;
        }
    }
    throw std::runtime_error("LeafNode->updateRecord: record with key: " + std::to_string(key) + " not found");
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
std::stringstream &
LeafNode<TKey, TValue, TDegree>::print(std::stringstream &ss) const {
    ss << "node" << this->fileOffset << "[xlabel=<<font color='#aaffaa'>" << this->fileOffset << "</font>> label=<{";
    auto records = this->getRecords();
    for (auto it = records.begin(); it != records.end(); it++)
        ss << "{" << it->first << '|' << "<font color='blue'>" << /*it->second*/"R" << "</font>}"
           << ((it == records.end() - 1) ? "" : "|");
    ss << "}>];\n";
    return ss;
}


template<typename TKey, typename TValue, size_t TDegree>
void
LeafNode<TKey, TValue, TDegree>::setRecords(KeysValuesIterator it1,
                                            KeysValuesIterator it2) {

    if (std::distance(it1, it2) > this->keys.size())
        throw std::runtime_error("Internal DB error: too much records in node to set");
    this->markChanged();
    auto keysI = keys.begin();
    auto valuesI = values.begin();
    std::for_each(it1, it2, [&keysI, &valuesI](auto x) {
        *keysI++ = std::move(x.first);
        *valuesI++ = std::move(x.second);
    });
    std::fill(keysI, keys.end(), std::nullopt);
    std::fill(valuesI, values.end(), std::nullopt);

}


template<typename TKey, typename TValue, size_t TDegree>
TKey
LeafNode<TKey, TValue, TDegree>::compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node,
                                                                  TKey const *const key,
                                                                  TValue const *const value,
                                                                  size_t nodeOffset) {

    if (node->nodeType() != NodeType::LEAF)
        throw std::runtime_error("Internal DB error: compensation failed, bad neighbour node type");

    if (key != nullptr && (std::find(this->keys.begin(), this->keys.end(), *key) != this->keys.end()))
        throw std::runtime_error("Record with given key already exists");


    auto otherNode = std::dynamic_pointer_cast<LeafNode>(node);
    auto aData = this->getRecords();
    auto bData = otherNode->getRecords();
    auto data = std::vector<std::pair<TKey, TValue>>();
    data.reserve(aData.size() + bData.size() + 1);

    // keys are sorted, hence we can merge
    std::merge(aData.begin(), aData.end(), bData.begin(), bData.end(), std::back_inserter(data),
               [](auto x, auto y) { return x.first < y.first; });

    // insert new key and value
    if (key != nullptr) {
        data.insert(std::upper_bound(data.begin(), data.end(), std::pair(*key, *value)), std::pair(*key, *value));
    }

    auto middleElementIterator = data.begin() + (data.size() - 1) / 2;

    // put first part of data and middle element to the left node
    this->setRecords(data.begin(), middleElementIterator + 1);

    // put rest in the right node
    otherNode->setRecords(middleElementIterator + 1, data.end());

    // return middle key
    return middleElementIterator->first;
}


template<typename TKey, typename TValue, size_t TDegree>
bool
LeafNode<TKey, TValue, TDegree>::contains(TKey const &key) const {
    for (auto &element : keys) {
        if (!element) return false;
        if (*element == key) return true;
    }
    return false;
}


template<typename TKey, typename TValue, size_t TDegree>
long
LeafNode<TKey, TValue, TDegree>::getLastRecordIndex() const {
    return std::distance(std::find_if(keys.rbegin(), keys.rend(), [](auto x) { return x; }), keys.rend()) - 1;
}


template<typename TKey, typename TValue, size_t TDegree>
NodeState
LeafNode<TKey, TValue, TDegree>::deleteRecord(TKey const &key) {
    NodeState result = NodeState::OK;
    auto const lastRecordIndex = this->getLastRecordIndex();
    auto const toDeleteRecordIndex = std::find(keys.begin(), keys.end(), key) - keys.begin();

    // delete key
    auto keysI = std::remove(keys.begin(), keys.end(), key);
    // delete record
    auto valuesI = std::move(values.begin() + toDeleteRecordIndex + 1, values.end(),
                             values.begin() + toDeleteRecordIndex);

    // fill empty space with std::nullopt
    std::fill(keysI, keys.end(), std::nullopt);
    std::fill(valuesI, values.end(), std::nullopt);


    // check if node contains valid number of elements
    if (this->fillKeysSize() < TDegree)
        result = (NodeState) (result | NodeState::TOO_SMALL);
    if (lastRecordIndex == toDeleteRecordIndex)
        result = (NodeState) (result | NodeState::DELETED_LAST);
    this->markChanged();
    return result;

}


template<typename TKey, typename TValue, size_t TDegree>
void
LeafNode<TKey, TValue, TDegree>::mergeWith(std::shared_ptr<Base> &other, TKey const *const) {
    if (other->nodeType() != NodeType::LEAF)
        throw std::runtime_error("Internal DB error: merge failed, bad neighbour other type");

    auto otherNode = std::dynamic_pointer_cast<LeafNode>(other);
    auto[otherKeysBegin, otherKeysEnd] = otherNode->getKeysRange();
    auto[otherValuesBegin, otherValuesEnd] = otherNode->getValuesRange();

    // move keys and values to the end of this node
    std::move(otherKeysBegin, otherKeysEnd, this->getKeysRange().second);
    std::move(otherValuesBegin, otherValuesEnd, this->getValuesRange().second);
    this->markChanged();
    otherNode->remove();
}


template<typename TKey, typename TValue, size_t TDegree>
typename LeafNode<TKey, TValue, TDegree>::KeysRange
LeafNode<TKey, TValue, TDegree>::getKeysRange() {
    return std::pair(keys.begin(),
                     std::find_if(keys.rbegin(), keys.rend(), [](auto x) { return x; }).base());
}


template<typename TKey, typename TValue, size_t TDegree>
typename LeafNode<TKey, TValue, TDegree>::ValuesRange
LeafNode<TKey, TValue, TDegree>::getValuesRange() {
    return std::pair(values.begin(),
                     std::find_if(values.rbegin(), values.rend(), [](auto x) { return x; }).base());
}


template<typename TKey, typename TValue, size_t TDegree>
typename LeafNode<TKey, TValue, TDegree>::KeysReverseRange
LeafNode<TKey, TValue, TDegree>::getKeysRangeReverse() {
    auto[b, e] = this->getKeysRange();
    return std::pair(std::reverse_iterator(e), std::reverse_iterator(b));
}


template<typename TKey, typename TValue, size_t TDegree>
typename LeafNode<TKey, TValue, TDegree>::ValuesReverseRange
LeafNode<TKey, TValue, TDegree>::getValuesRangeReverse() {
    auto[b, e] = this->getValuesRange();
    return std::pair(std::reverse_iterator(e), std::reverse_iterator(b));
}


#endif //SBD2_LEAF_NODE_HH
