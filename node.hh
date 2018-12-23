//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_NODE_HH
#define SBD2_NODE_HH

#include <cstddef>
#include <memory>
#include <iostream>
#include <bitset>
#include "dbms.hh"
#include "tools.hh"

class File;

template<typename TKey, typename TValue> class Node;

template<typename TKey, typename TValue, size_t TDegree> class InnerNode;

template<typename TKey, typename TValue, size_t TDegree> class LeafNode;

template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream &, Node<TKey, TValue> const &);
template<typename TKey, typename TValue> std::ostream &operator<<(std::stringstream &, Node<TKey, TValue> const &);

enum class NodeType { INNER, LEAF };
enum NodeState:uint8_t {OK = 0, DELETED_LAST = 1, TOO_SMALL = 2};

template<typename TKey, typename TValue>
class Node {
public:
    Node() = delete;
    Node(size_t fileOffset, File &file, std::shared_ptr<Node> parent = nullptr);
    virtual ~Node();

    friend std::ostream &operator<<<TKey, TValue>(std::ostream &os, Node<TKey, TValue> const &node);
    friend std::ostream &operator<<<TKey, TValue>(std::stringstream &ss, Node<TKey, TValue> const &node);
    virtual NodeType nodeType() const = 0;
    virtual TKey compensateWithAndReturnMiddleKey(std::shared_ptr<Node> node, TKey const *const key,
                                                  TValue const *const value,
                                                  size_t nodeOffset) = 0;
    virtual void mergeWith(std::shared_ptr<Node> &node) = 0;
    virtual std::ostream &print(std::ostream &o) const = 0;
    virtual std::stringstream &print(std::stringstream &ss) const = 0;
    virtual bool contains(TKey const &key) const = 0;
    Node &load(std::vector<char> const &bytes);
    Node &unload();
    Node &markEmpty();
    Node &markChanged();
    virtual bool full() const = 0;
    std::shared_ptr<Node> parent;
    size_t fileOffset{};
    static uint64_t GetCurrentNodesCount() { return currentNodesCount; }
    static uint64_t GetMaxNodesCount() { return maxNodesCount; }
    static void ResetCounters();
    virtual size_t degree() = 0;
    virtual size_t fillKeysSize() const = 0;
    virtual bool operator<(Node const &rhs) const = 0;
    bool isChanged() const { return changed; }
    bool isLoaded() const { return loaded; }

protected:
    virtual size_t elementsSize() const = 0;
    virtual size_t bytesSize() const = 0;
    virtual std::vector<uint8_t> getData() = 0;
    std::vector<char> serialize();
    virtual Node &deserialize(std::vector<char> const &bytes) = 0;
    void remove();
    File &file;
    bool empty;
    bool changed;
    bool loaded;
    void incCounter();
    void decCounter();
    inline static uint64_t currentNodesCount = 0;
    inline static uint64_t maxNodesCount = 0;
};


template<typename TKey, typename TValue>
Node<TKey, TValue>::Node(size_t const fileOffset, File &file, std::shared_ptr<Node> parent)
        : file(file), fileOffset(fileOffset), parent(std::move(parent)), empty(false), changed(false),
          loaded(false) {
    Tools::debug([this] { std::clog << "Created node: " << this->fileOffset << '\n'; }, 2);
    incCounter();
}


template<typename TKey, typename TValue>
void Node<TKey, TValue>::incCounter() {
    currentNodesCount++;
    if (currentNodesCount > maxNodesCount) maxNodesCount = currentNodesCount;
}


template<typename TKey, typename TValue>
void Node<TKey, TValue>::decCounter() {
    currentNodesCount--;
}


template<typename TKey, typename TValue> Node<TKey, TValue>::~Node() {
    Tools::debug([this] { std::clog << "Exiting node: " << fileOffset << '\n'; }, 2);
    decCounter();
}


template<typename TKey, typename TValue> void Node<TKey, TValue>::remove() {
    Tools::debug([this] { std::clog << "Removing node: " << fileOffset << '\n'; }, 2);
    this->empty = true;
    this->changed = true;
    this->unload();
}


template<typename TKey, typename TValue> std::vector<char> Node<TKey, TValue>::serialize() {
    auto result = std::vector<char>{};
    auto size = this->bytesSize() + 1;
    result.reserve(size);
    std::bitset<8> headerByte = 0;
    headerByte[0] = this->empty;
    headerByte[1] = static_cast<bool>(this->nodeType());
    result.emplace_back(static_cast<uint8_t>(headerByte.to_ulong()));
    auto data = this->getData();
    std::copy(data.begin(), data.end(), std::back_inserter(result));
    return result;
}


template<typename TKey, typename TValue>
Node<TKey, TValue> &
Node<TKey, TValue>::load(std::vector<char> const &bytes) {
    Tools::debug([this] { std::clog << "Constructing node from bytes: " << this->fileOffset << '\n'; }, 3);
    this->deserialize(bytes);
    this->loaded = true;
    this->changed = false;
    return *this;
}


template<typename TKey, typename TValue>
Node<TKey, TValue> &
Node<TKey, TValue>::unload() {
    if (!changed) {
        Tools::debug([this] { std::clog << "Node at " << this->fileOffset << " unchanged\n"; }, 3);
        return *this;
    }
    Tools::debug([this] { std::clog << "Unloading node at: " << this->fileOffset << '\n'; }, 3);
    auto bytes = this->serialize();
    if (bytes.size() != this->bytesSize() + 1) {
        throw std::runtime_error("Sizes do not match");
    }
    this->file.write(this->fileOffset, bytes);
    if (!this->file.good())Tools::debug([] { std::clog << "Error while writing node\n"; });
    this->changed = false;
    this->loaded = true;
    return *this;
}


template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream &os, Node<TKey, TValue> const &node) {
    return node.print(os);
}


template<typename TKey, typename TValue>
std::ostream &operator<<(std::stringstream &ss, Node<TKey, TValue> const &node) {
    return node.print(ss);
}


template<typename TKey, typename TValue>
Node<TKey, TValue> &Node<TKey, TValue>::markEmpty() {
    this->changed = true;
    this->empty = true;
    return *this;
}


template<typename TKey, typename TValue>
Node<TKey, TValue> &Node<TKey, TValue>::markChanged() {
    this->changed = true;
    return *this;
}


template<typename TKey, typename TValue>
void Node<TKey, TValue>::ResetCounters() {
    maxNodesCount = currentNodesCount = 0;

}

#endif //SBD2_NODE_HH
