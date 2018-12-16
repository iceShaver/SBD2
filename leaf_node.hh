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

public:
    LeafNode(size_t fileOffset, File &file, std::shared_ptr<Base> parent = nullptr);
    ~LeafNode() override;

    KeysCollection keys;
    ValuesCollection values;
    static size_t BytesSize();
    LeafNode &insert(TKey const &key, TValue const &value);
    std::optional<TValue> readRecord(TKey const &key) const;
    void updateRecord(TKey const &key, TValue const &value);
    bool full() const override;
    bool contains(TKey const &key) const override;
    TKey compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const &key, TValue const &value,
                                          size_t nodeOffset) override;


    std::vector<std::pair<TKey, TValue>> getRecords() const;
    LeafNode &setRecords(std::vector<std::pair<TKey, TValue>> const &records);
    LeafNode &setRecords(typename std::vector<std::pair<TKey, TValue>>::iterator it1,
                         typename std::vector<std::pair<TKey, TValue>>::iterator it2);
    std::stringstream &print(std::stringstream &ss) const override;

private:
    std::ostream &print(std::ostream &o) const override;
    Node<TKey, TValue> &deserialize(std::vector<char> const &bytes) override;
    std::vector<uint8_t> getData() override;
    size_t elementsSize() const override;
    size_t fillElementsSize() const override;
    size_t bytesSize() const override;
    NodeType nodeType() const override;
    constexpr auto ElementsSize() const noexcept;
};


template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree>::LeafNode(size_t fileOffset, File &file,
                                          std::shared_ptr<Base> parent) : Base(fileOffset, file, parent) {}


template<typename TKey, typename TValue, size_t TDegree>
size_t
LeafNode<TKey, TValue, TDegree>::elementsSize() const {
    return ElementsSize();
}


template<typename TKey, typename TValue, size_t TDegree>
constexpr auto
LeafNode<TKey, TValue, TDegree>::ElementsSize() const noexcept {
    return this->keys.size() + this->values.size() + 1;
}


template<typename TKey, typename TValue, size_t TDegree>
NodeType
LeafNode<TKey, TValue, TDegree>::nodeType() const { return NodeType::LEAF; }


template<typename TKey, typename TValue, size_t TDegree>
std::vector<uint8_t>
LeafNode<TKey, TValue, TDegree>::getData() {
    auto result = std::vector<uint8_t>();
    auto keysByteArray = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    auto valuesByteArray = (std::array<uint8_t, sizeof(this->values)> *) (this->values.data());
    result.reserve(keysByteArray->size() + valuesByteArray->size());
    std::copy(keysByteArray->begin(), keysByteArray->end(), std::back_inserter(result));
    std::copy(valuesByteArray->begin(), valuesByteArray->end(), std::back_inserter(result));
    return std::move(result);
}


template<typename TKey, typename TValue, size_t TDegree>
Node<TKey, TValue> &
LeafNode<TKey, TValue, TDegree>::deserialize(std::vector<char> const &bytes) {
    this->changed = true;
    auto valuesBytePtr = (std::array<uint8_t, sizeof(this->values)> *) this->values.data();
    auto keysBytePtr = (std::array<uint8_t, sizeof(this->keys)> *) this->keys.data();
    std::copy_n(bytes.begin(), keysBytePtr->size(), keysBytePtr->begin());
    std::copy_n(bytes.begin() + keysBytePtr->size(), valuesBytePtr->size(), valuesBytePtr->begin());
    return *this;
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
LeafNode<TKey, TValue, TDegree>::bytesSize() const {
    return BytesSize();
}


template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree>::~LeafNode() {
    this->unload(); // don't touch this!
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
LeafNode<TKey, TValue, TDegree>::BytesSize() {
    return sizeof(KeysCollection) + sizeof(ValuesCollection);
}


template<typename TKey, typename TValue, size_t TDegree>
size_t
LeafNode<TKey, TValue, TDegree>::fillElementsSize() const {
    return static_cast<size_t>(std::count_if(this->keys.begin(), this->keys.end(),
                                             [](auto x) { return x != std::nullopt; }));
}


template<typename TKey, typename TValue, size_t TDegree>
bool
LeafNode<TKey, TValue, TDegree>::full() const {
    return std::find(this->keys.begin(), this->keys.end(), std::nullopt) == this->keys.end();
}


template<typename TKey, typename TValue, size_t TDegree>
LeafNode<TKey, TValue, TDegree> &
LeafNode<TKey, TValue, TDegree>::insert(TKey const &key, TValue const &value) {

    if (this->full()) throw std::runtime_error("Tried to add element to full node");
    this->changed = true;
    auto records = this->getRecords();
    records.insert(std::upper_bound(records.begin(), records.end(), std::pair(key, value),
                                    [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
    this->setRecords(records);
    return *this;
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
            values[i] = value;
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
    ss << "node" << this->fileOffset << "[xlabel=<" << this->fileOffset << "> label=<";
    auto records = this->getRecords();
    for (auto it = records.begin(); it != records.end(); it++)
        ss << it->first << '|' << "<font color='blue'>" << /*it->second*/"R" << "</font>"
           << ((it == records.end() - 1) ? "" : "|");
    ss << ">];\n";
    return ss;
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
    auto distance = std::distance(it1, it2);
    if (distance > this->keys.size())
        throw std::runtime_error("Internal DB error: too much records in node to set");
    this->changed = true;
    KeysCollection keys;
    ValuesCollection values;
    std::transform(it1, it2, keys.begin(), [](auto x) { return x.first; });
    std::transform(it1, it2, values.begin(), [](auto x) { return x.second; });
    this->keys = std::move(keys);
    this->values = std::move(values);
    return *this;
}


template<typename TKey, typename TValue, size_t TDegree>
TKey
LeafNode<TKey, TValue, TDegree>::compensateWithAndReturnMiddleKey(std::shared_ptr<Base> node, TKey const &key,
                                                                  TValue const &value, size_t nodeOffset) {

    if (node->nodeType() != NodeType::LEAF)
        throw std::runtime_error("Internal DB error: compensation failed, bad neighbour node type");
    if (std::find(this->keys.begin(), this->keys.end(), key) != this->keys.end())
        throw std::runtime_error("Record with given key already exists");


    auto otherNode = std::dynamic_pointer_cast<LeafNode>(node);
    std::vector<std::pair<TKey, TValue>> data;

    std::vector<std::pair<TKey, TValue>> aData = this->getRecords();
    std::vector<std::pair<TKey, TValue>> bData = otherNode->getRecords();

    // determine order (which node is left, right) (contains smaller keys), if otherNode is newly created, then it is right
    bool order = bData.size() != 0 ? aData[0].first < bData[0].first : true;
    auto nodes = order ? std::pair(this, otherNode.get())
                       : std::pair(otherNode.get(), this);
    // keys are sorted, hence we can merge
    std::merge(aData.begin(), aData.end(), bData.begin(), bData.end(), std::back_inserter(data),
               [](auto x, auto y) { return x.first < y.first; });

    // insert new key
    data.insert(std::upper_bound(data.begin(), data.end(), std::pair(key, value),
                                 [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
    auto middleElementIterator = data.begin() + (data.size() - 1) / 2;

    // put first part of data and middle element to the left node
    nodes.first->setRecords(data.begin(), middleElementIterator + 1);

    // put rest in the right node
    nodes.second->setRecords(middleElementIterator + 1, data.end());

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


#endif //SBD2_LEAF_NODE_HH
