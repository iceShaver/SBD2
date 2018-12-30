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


template<typename TKey, typename TValue> auto &operator<<(std::ostream &s, Node<TKey, TValue> &node) {
    return node.print(s);
}

template<typename TKey, typename TValue> auto &operator<<(std::stringstream &s, Node<TKey, TValue> &node) {
    return node.print(s);
}

using NodeOffset = size_t;
using Byte = char;

enum class NodeType { INNER, LEAF };
enum NodeState : uint8_t { OK = 0, DELETED_LAST = 1, TOO_SMALL = 2 };


template<typename TKey, typename TValue>
class Node {

    friend auto &operator<<<TKey, TValue>(std::ostream &os, Node<TKey, TValue> &node);
    friend auto &operator<<<TKey, TValue>(std::stringstream &ss, Node<TKey, TValue> &node);
    template<typename, typename, size_t, size_t> friend class BPlusTree;


public:
    Node() = delete;
    Node(size_t fileOffset, File &file, std::shared_ptr<Node> parent = nullptr);
    virtual ~Node();


    virtual auto nodeType() const -> NodeType = 0;
    virtual auto compensateWithAndReturnMiddleKey(std::shared_ptr<Node> node, TKey const *key,
                                                  TValue const *value,
                                                  size_t nodeOffset) -> TKey = 0;
    virtual auto mergeWith(std::shared_ptr<Node> &node, TKey const *key = nullptr) -> void = 0;
    virtual auto print(std::ostream &o) -> std::ostream & = 0;
    virtual auto print(std::stringstream &ss) -> std::stringstream & = 0;
    virtual auto contains(TKey const &key) const -> bool = 0;
    virtual auto full() const -> bool = 0;
    virtual auto degree() -> size_t = 0;
    virtual auto fillKeysSize() const -> size_t = 0;

    auto load(std::vector<char> const &bytes) -> void;
    auto unload() -> void;
    auto markEmpty() { changed = true, empty = true; };
    auto markChanged() { changed = true; };

    auto isChanged() const { return changed; }
    auto isLoaded() const { return loaded; }

    static auto GetCurrentNodesCount() { return currentNodesCount; }
    static auto GetMaxNodesCount() { return maxNodesCount; }
    static auto ResetCounters() { maxNodesCount = currentNodesCount = 0; };


    std::shared_ptr<Node> parent;
    size_t fileOffset{};

protected:

    virtual size_t elementsSize() const = 0;
    virtual size_t bytesSize() const = 0;
    virtual std::vector<Byte> getData() = 0;
    std::vector<Byte> serialize();
    virtual auto deserialize(std::vector<Byte> const &bytes) -> void = 0;
    void remove();
    void incCounter() { maxNodesCount = std::max(maxNodesCount, ++currentNodesCount); };
    void decCounter() { --currentNodesCount; };

    File &file;
    bool empty;
    bool changed;
    bool loaded;

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
Node<TKey, TValue>::~Node() {
    Tools::debug([this] { std::clog << "Exiting node: " << fileOffset << '\n'; }, 2);
    decCounter();
}


template<typename TKey, typename TValue>
auto Node<TKey, TValue>::remove() -> void {
    Tools::debug([this] { std::clog << "Removing node: " << fileOffset << '\n'; }, 2);
    this->empty = true;
    this->changed = true;
    this->unload();
}


template<typename TKey, typename TValue>
auto Node<TKey, TValue>::serialize() -> std::vector<char> {
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
auto Node<TKey, TValue>::load(std::vector<char> const &bytes) -> void{
    Tools::debug([this] { std::clog << "Constructing node from bytes: " << this->fileOffset << '\n'; }, 3);
    this->deserialize(bytes);
    this->loaded = true;
    this->changed = false;
}


template<typename TKey, typename TValue>
auto Node<TKey, TValue>::unload() -> void{
    if (!changed) {
        Tools::debug([this] { std::clog << "Node at " << this->fileOffset << " unchanged\n"; }, 3);
        return;
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
}


#endif //SBD2_NODE_HH
