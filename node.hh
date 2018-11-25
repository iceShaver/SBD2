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

using std::cout;
enum class NodeType { INNER, LEAF };

template <typename TKey, typename TValue> class Node;

template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream&, Node<TKey, TValue> const &);

template<typename TKey, typename TValue> class Node {
public:

    Node(size_t fileOffset, std::fstream &fileHandle, std::weak_ptr<Node> const &parent = std::weak_ptr<Node>());
    virtual ~Node();

    friend std::ostream &operator<<<TKey, TValue>(std::ostream &os, Node<TKey, TValue> const &node);

    virtual NodeType nodeType() const = 0;
    virtual std::ostream& printData(std::ostream&o) const = 0;
    virtual void load();
    virtual void unload();

protected:

    virtual size_t elementsSize() const = 0;
    virtual size_t bytesSize() const = 0;
    virtual std::vector<uint8_t> getData() = 0;
    std::vector<uint8_t> serialize();
    virtual Node &deserialize(std::vector<uint8_t> const &bytes) = 0;
    void remove();
    std::fstream &fileHandle;
    size_t fileOffset{};
    std::weak_ptr<Node> parent;
    bool empty;
};






template<typename TKey, typename TValue>
Node<TKey, TValue>::Node(size_t const fileOffset, std::fstream &fileHandle, std::weak_ptr<Node> const &parent)
        : fileHandle(fileHandle), fileOffset(fileOffset), parent(parent), empty(false) {
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

template<typename TKey, typename TValue>std::vector<uint8_t> Node<TKey, TValue>::serialize() {
    auto result = std::vector<uint8_t>{};
    result.reserve(this->bytesSize());
    std::bitset<8> headerByte = 0;
    headerByte[0] = this->empty;
    headerByte[1] = static_cast<bool>(this->nodeType());
    result[0] = static_cast<uint8_t>(headerByte.to_ulong());
    auto data = this->getData();
    std::copy(data.begin(), data.end(), result.begin() + 1);
    return result;
}

template<typename TKey, typename TValue>void Node<TKey, TValue>::load() {
    this->fileHandle.seekg(this->fileOffset);
    auto buffer = std::vector<uint8_t>();
    buffer.resize(this->bytesSize());
    this->fileHandle.read(reinterpret_cast<char*>(buffer.data()), this->bytesSize());
    this->deserialize(buffer);

}

template<typename TKey, typename TValue> void Node<TKey, TValue>::unload() {
    // TODO: check if page changed
    this->fileHandle.seekp(this->fileOffset);
    auto bytes = this->serialize();
    this->fileHandle.write(reinterpret_cast<char*>(bytes.data()), this->bytesSize());
}
template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream &os, Node<TKey, TValue> const &node) {
    os << "Node: " << node.fileOffset << '\n';
    return node.printData(os);
}


#endif //SBD2_NODE_HH
