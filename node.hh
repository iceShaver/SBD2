//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_NODE_HH
#define SBD2_NODE_HH

#include <cstddef>
#include <memory>
#include <iostream>
#include <bitset>
#include "tools.hh"

template<typename TKey, typename TValue, size_t TDegree> class InnerNode;

template<typename TKey, typename TValue, size_t TDegree> class LeafNode;

using std::cout;
enum class NodeType { INNER, LEAF };

template<typename TKey, typename TValue> class Node;

template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream &, Node<TKey, TValue> const &);




template<typename TKey, typename TValue> class Node {
public:
    Node() = delete;
    Node(size_t fileOffset, std::fstream &fileHandle, std::weak_ptr<Node> const &parent = std::weak_ptr<Node>());
    virtual ~Node();

    friend std::ostream &operator<<<TKey, TValue>(std::ostream &os, Node<TKey, TValue> const &node);

    virtual NodeType nodeType() const = 0;
    virtual std::ostream &printData(std::ostream &o) const = 0;
    Node &load();
    Node &unload();
    Node &markEmpty();

protected:

    virtual size_t elementsSize() const = 0;
    virtual size_t fillElementsSize() const = 0;
    virtual bool isFull() const = 0;
    virtual size_t bytesSize() const = 0;
    virtual std::vector<uint8_t> getData() = 0;
    std::vector<uint8_t> serialize();
    virtual Node &deserialize(std::vector<uint8_t> const &bytes) = 0;
    void remove();
    std::fstream &fileHandle;
    size_t fileOffset{};
    std::weak_ptr<Node> parent;
    bool empty;
    bool changed;
};





template<typename TKey, typename TValue>
Node<TKey, TValue>::Node(size_t const fileOffset, std::fstream &fileHandle, std::weak_ptr<Node> const &parent)
        : fileHandle(fileHandle), fileOffset(fileOffset), parent(parent), empty(false), changed(true) {
    debug([this] { std::clog << "Created node: " << this->fileOffset << '\n'; });
}


template<typename TKey, typename TValue> Node<TKey, TValue>::~Node() {
    debug([this] { std::clog << "Exiting node: " << fileOffset << '\n'; });
}

template<typename TKey, typename TValue> void Node<TKey, TValue>::remove() {
    debug([this] { std::clog << "Removing node: " << fileOffset << '\n'; });
    this->empty = true;
    this->uload();
}

template<typename TKey, typename TValue> std::vector<uint8_t> Node<TKey, TValue>::serialize() {
    auto result = std::vector<uint8_t>{};
    result.reserve(this->bytesSize() + 1);
    std::bitset<8> headerByte = 0;
    headerByte[0] = this->empty;
    headerByte[1] = static_cast<bool>(this->nodeType());
    result[0] = static_cast<uint8_t>(headerByte.to_ulong());
    auto data = this->getData();
    std::copy(data.begin(), data.end(), result.begin() + 1);
    return result;
}

template<typename TKey, typename TValue> Node<TKey, TValue> &Node<TKey, TValue>::load() {
    this->fileHandle.seekg(this->fileOffset);
    auto buffer = std::vector<uint8_t>();
    buffer.resize(this->bytesSize());
    char header;
    this->fileHandle.read(&header, 1);
    this->fileHandle.read(reinterpret_cast<char *>(buffer.data()), this->bytesSize());
    this->deserialize(buffer);
    this->changed = false;
    return *this;
}

template<typename TKey, typename TValue> Node<TKey, TValue> &Node<TKey, TValue>::unload() {
    if (!changed) {
        debug([this] { std::clog << "Node unchanged\n"; });
        return *this;
    }
    debug([this] { std::clog << "Node changed, saving at: " << this->fileOffset << '\n'; });
    this->fileHandle.seekp(this->fileOffset);
    auto bytes = this->serialize();
    fileHandle.clear();
    this->fileHandle.write(reinterpret_cast<char *>(bytes.data()), this->bytesSize() + 1);
    if (!this->fileHandle.good())debug([] { std::clog << "Error while writing node\n"; });
    long ile = this->fileHandle.tellp() - fileOffset;
    return *this;
}

template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream &os, Node<TKey, TValue> const &node) {
    os << "Node: " << node.fileOffset << '\n';
    return node.printData(os);
}

template<typename TKey, typename TValue>
Node<TKey, TValue> &Node<TKey, TValue>::markEmpty() {
    this->changed = true;
    this->empty = true;
    return *this;
}


#endif //SBD2_NODE_HH
