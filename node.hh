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
    Node(size_t fileOffset, std::fstream &fileHandle, std::shared_ptr<Node> parent = nullptr);
    virtual ~Node();

    friend std::ostream &operator<<<TKey, TValue>(std::ostream &os, Node<TKey, TValue> const &node);

    virtual NodeType nodeType() const = 0;

    virtual TKey compensateWithAndReturnMiddleKey(std::shared_ptr<Node> node, TKey const &key, TValue const &value, size_t nodeOffset) = 0;

    virtual std::ostream &print(std::ostream &o) const = 0;
    Node &load();
    Node &unload();
    Node &markEmpty();
    Node& markChanged();
    virtual bool full() const = 0;
    //virtual void changeKey(size_t aPtr,size_t bPtr, TKey const & key) = 0;
    std::shared_ptr<Node> parent;
    size_t fileOffset{};

protected:

    virtual size_t elementsSize() const = 0;
    virtual size_t fillElementsSize() const = 0;
    virtual size_t bytesSize() const = 0;
    virtual std::vector<uint8_t> getData() = 0;
    std::vector<uint8_t> serialize();
    virtual Node &deserialize(std::vector<uint8_t> const &bytes) = 0;
    void printNodeAndDescendants();
    void remove();
    std::fstream &fileHandle;
    bool empty;
    bool changed;
    bool loaded;
};


template<typename TKey, typename TValue>
Node<TKey, TValue>::Node(size_t const fileOffset, std::fstream &fileHandle, std::shared_ptr<Node> parent)
        : fileHandle(fileHandle), fileOffset(fileOffset), parent(std::move(parent)), empty(false), changed(false),
          loaded(false) {
    debug([this] { std::clog << "Created node: " << this->fileOffset << '\n'; });
}


template<typename TKey, typename TValue> Node<TKey, TValue>::~Node() {
    debug([this] { std::clog << "Exiting node: " << fileOffset << '\n'; });
}

template<typename TKey, typename TValue> void Node<TKey, TValue>::remove() {
    debug([this] { std::clog << "Removing node: " << fileOffset << '\n'; });
    this->empty = true;
    this->changed = true;
    this->unload();
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
    debug([this] { std::clog << "Loading node at: " << this->fileOffset << '\n'; }, 3);
    this->fileHandle.flush();
    if (!this->fileHandle.good())
        throw std::runtime_error("loading node error");
    this->fileHandle.seekg(this->fileOffset);
    auto size = this->bytesSize();
    auto buffer = std::vector<uint8_t>();
    buffer.resize(this->bytesSize());
    char header;
    if (!this->fileHandle.good() || this->fileHandle.bad())
        throw std::runtime_error("loading node error");
    this->fileHandle.read(&header, 1);
    this->fileHandle.read(reinterpret_cast<char *>(buffer.data()), this->bytesSize());
    if (!this->fileHandle.good())
        throw std::runtime_error("loading node error");
    this->deserialize(buffer);
    this->changed = false;
    this->loaded = true;
    return *this;
}

template<typename TKey, typename TValue> Node<TKey, TValue> &Node<TKey, TValue>::unload() {
    if (!changed) {
        debug([this] { std::clog << "Node at " << this->fileOffset << " unchanged\n"; }, 3);
        return *this;
    }
    debug([this] { std::clog << "Unloading node at: " << this->fileOffset << '\n'; }, 3);
    this->fileHandle.seekp(this->fileOffset);
    auto bytes = this->serialize();
    fileHandle.clear();
    this->fileHandle.write(reinterpret_cast<char *>(bytes.data()), this->bytesSize());
    if (!this->fileHandle.good())debug([] { std::clog << "Error while writing node\n"; });
    this->changed = false;
    this->loaded = false;
    return *this;
}

template<typename TKey, typename TValue> std::ostream &operator<<(std::ostream &os, Node<TKey, TValue> const &node) {
    return node.print(os);
}

template<typename TKey, typename TValue>
Node<TKey, TValue> &Node<TKey, TValue>::markEmpty() {
    this->changed = true;
    this->empty = true;
    return *this;
}
template<typename TKey, typename TValue>
void Node<TKey, TValue>::printNodeAndDescendants() {

}
template<typename TKey, typename TValue>
Node<TKey, TValue> &Node<TKey, TValue>::markChanged() {
    this->changed = true;
    return *this;
}


#endif //SBD2_NODE_HH
