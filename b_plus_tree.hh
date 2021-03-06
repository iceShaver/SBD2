
//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_B_PLUS_TREE_HH
#define SBD2_B_PLUS_TREE_HH

#include <string_view>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <memory>
#include <utility>
#include <graphviz/gvc.h>
#include "node.hh"
#include "inner_node.hh"
#include "leaf_node.hh"
#include "record.hh"
#include "tools.hh"
#include "file.hh"

using namespace std::string_literals;
namespace fs = std::filesystem;

class Dbms;

enum class OpenMode { USE_EXISTING, CREATE_NEW };
enum class IteratorT { BEGIN, END };


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree;


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::ostream &operator<<(std::ostream &o, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &t) {
    return o << *t.root << '\n';
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree final {
    struct ConfigHeader {
        uint64_t rootOffset = 0;
        uint64_t innerNodeDegree = 0;
        uint64_t leafNodeDegree = 0;
    };

    class Iterator;
    class ForwardIterator;
    class ReverseIterator;

    using ANode = Node<TKey, TValue>;
    using AInnerNode = InnerNode<TKey, TValue, TInnerNodeDegree>;
    using ALeafNode = LeafNode<TKey, TValue, TLeafNodeDegree>;

    friend Iterator;
    friend Dbms;
    friend std::ostream &operator<<<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>(
            std::ostream &os,
            BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree);


public:
    BPlusTree() = delete;
    BPlusTree(BPlusTree &&) = delete;
    BPlusTree(BPlusTree const &) = delete;
    BPlusTree &operator=(BPlusTree &&) = delete;
    BPlusTree &operator=(BPlusTree const &) = delete;
    ~BPlusTree();

    explicit BPlusTree(fs::path filePath, OpenMode openMode = OpenMode::USE_EXISTING);

    auto readNode(size_t fileOffset) -> std::shared_ptr<ANode>;
    auto AllocateDiskMemory(NodeType nodeType) -> size_t;

    // CRUD operations
    auto createRecord(TKey const &key, TValue const &value) -> void;
    auto readRecord(TKey const &key) -> std::optional<TValue>;
    auto updateRecord(TKey const &key, TValue const &value) -> void;
    auto deleteRecord(TKey const &key) -> void;

    auto findProperDescendantOffset(std::shared_ptr<ANode> node, TKey const &key) -> NodeOffset;
    auto findProperLeaf(TKey const &key) -> std::shared_ptr<ALeafNode>;

    auto tryCompensateAndAdd(std::shared_ptr<ANode> node,
                             TKey const *key = nullptr,
                             TValue const *value = nullptr,
                             size_t nodeOffset = 0) -> bool;

    auto splitAndAddRecord(std::shared_ptr<ANode> node,
                           TKey const &key, TValue const &value, size_t addedNodeOffset = 0) -> void;
    auto merge(std::shared_ptr<ANode> node, TKey const *key = nullptr) -> void;
    auto getNodeNeighbours(std::shared_ptr<ANode> node) -> std::pair<std::shared_ptr<ANode>, std::shared_ptr<ANode>>;
    auto getFirstLeaf() -> std::shared_ptr<ALeafNode>;
    auto getLastLeaf() -> std::shared_ptr<ALeafNode>;
    auto unload() -> void { root->unload(), updateConfigHeader(); }


    auto draw() -> void;
    auto print() -> void;
    auto printFile() -> void;
    auto gvcPrintTree() -> std::stringstream;
    auto printNodeAndDescendants(std::shared_ptr<ANode> node) -> void;
    auto gvcPrintNodeAndDescendants(std::shared_ptr<ANode> node, std::stringstream &ss) -> std::stringstream &;


    auto name() const -> std::string;
    constexpr auto innerNodeDegree() const -> size_t { return TInnerNodeDegree; }
    constexpr auto leafNodeDegree() const -> size_t { return TLeafNodeDegree; }
    auto getSessionDiskReadsCout() const -> uint64_t { return sessionDiskReadsCount; }
    auto getSessionDiskWritesCount() const -> uint64_t { return sessionDiskWritesCount; }
    auto getCurrentOperationDiskReadsCount() const -> uint64_t { return currentOperationDiskReadsCount; }
    auto getCurrentOperationDiskWritesCount() const -> uint64_t { return currentOperationDiskWritesCount; }
    auto getHeight() -> uint64_t;
    auto getRecordsNumber() -> uint64_t;
    auto getNodesCount() -> std::pair<uint64_t, uint64_t>;
    auto disableCounters() -> void { countersEnabled = false; }
    auto enableCounters() -> void { countersEnabled = true; }

    auto begin() -> ForwardIterator const { return ForwardIterator(getFirstLeaf(), this, IteratorT::BEGIN); }
    auto end() -> ForwardIterator const { return ForwardIterator(); }
    auto rbegin() -> ReverseIterator const { return BPlusTree::ReverseIterator(getLastLeaf(), this, IteratorT::BEGIN); }
    auto rend() -> ReverseIterator const { return BPlusTree::ReverseIterator(); };


private:
    auto getNodesCount(std::shared_ptr<ANode> node, std::pair<uint64_t, uint64_t> &counters) -> void;
    auto resetOpCounters() -> void { currentOperationDiskWritesCount = currentOperationDiskReadsCount = 0; }
    auto incrementWriteOperationsCounters() -> void;
    auto incrementReadOperationsCounters() -> void;
    auto resetCounters() -> void;
    auto updateConfigHeader() -> void;


    uint64_t sessionDiskReadsCount = 0;
    uint64_t sessionDiskWritesCount = 0;
    uint64_t currentOperationDiskReadsCount = 0;
    uint64_t currentOperationDiskWritesCount = 0;
    bool countersEnabled = true;
    fs::path filePath;
    File file;
    std::shared_ptr<ANode> root;
    ConfigHeader configHeader;
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator {
public:
    // types required for STL
    using difference_type = long long;
    using value_type = std::pair<TKey, TValue>;
    using pointer = const std::pair<TKey, TValue> *;
    using reference = std::pair<TKey, TValue> &;
    using iterator_category = std::bidirectional_iterator_tag;

    friend BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>;


public:
    auto operator*() -> value_type;
    auto operator==(Iterator const &other) const -> bool;
    auto operator!=(Iterator const &other) const -> bool { return !(*this == other); }

protected:
    Iterator() = default;
    Iterator(std::shared_ptr<ALeafNode> node,
             BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree);

    auto inc() -> void; // ++x;
    auto dec() -> void; // x++


    bool afterEnd = false;
    bool beforeBegin = false;
    std::shared_ptr<ALeafNode> node = nullptr;
    size_t i = 0;
    BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree = nullptr;
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ForwardIterator
        : public BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator {
    using Base = BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator;

public:
    ForwardIterator(
            std::shared_ptr<ALeafNode> node,
            BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree,
            IteratorT iteratorType) : Base(node, tree) {
        if (iteratorType == IteratorT::END) {
            this->afterEnd = true;
            this->i = this->node->getLastRecordIndex();
        }
    }

    ForwardIterator() { this->afterEnd = true; }


    auto operator--() -> ForwardIterator & { return this->dec(), *this; } // --x
    auto operator++() -> ForwardIterator & { return this->inc(), *this; }; // ++x

    auto operator--(int) -> ForwardIterator const {
        auto const tmp = *this;
        this->dec();
        return tmp;
    } // x--


    auto operator++(int) -> ForwardIterator const {
        auto const tmp = *this;
        this->inc();
        return tmp;
    }; // x++
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ReverseIterator
        : public BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator {
    using Base = BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator;
public:
    ReverseIterator(
            std::shared_ptr<ALeafNode> node,
            BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree,
            IteratorT iteratorType) : Base(node, tree) {
        this->i = this->node->getLastRecordIndex();
        if (iteratorType == IteratorT::END) {
            this->beforeBegin = true;
            this->i = 0;
        }
    }

    ReverseIterator() { this->beforeBegin = true; };

    auto operator++() -> ReverseIterator & { return this->dec(), *this; } //++x
    auto operator--() -> ReverseIterator & { return this->inc(), *this; } //--x

    auto operator--(int) -> ReverseIterator const {
        auto const tmp = *this;
        this->inc();
        return tmp;
    }; // x--

    auto operator++(int) -> ReverseIterator const {
        auto const tmp = *this;
        this->dec();
        return tmp;
    }; // x++
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::Iterator(
        std::shared_ptr<ALeafNode> node,
        BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree)
        : afterEnd(node->fillKeysSize() == 0),
          beforeBegin(afterEnd),
          node(std::move(node)),
          i(0),
          tree(std::move(tree)) {

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::inc() -> void {

    if (beforeBegin) {
        beforeBegin = false;
        return;
    }
    // next record is in the same node
    if (i < this->node->keys.size() - 1 && this->node->keys[i + 1] != std::nullopt) {
        i++;
        return;
    }

    // if no parent then afterEnd iterator
    if (node->parent == nullptr) {
        afterEnd = true;
        return;
    }
    std::shared_ptr<ANode> tmpNodePtr = this->node;


    // go up and search first right neighbour, if so get most left node
    while (true) {
        if (tmpNodePtr->parent == nullptr) {
            afterEnd = true;
            return;
        }

        auto parent = std::dynamic_pointer_cast<AInnerNode>(tmpNodePtr->parent);
        auto nextDescendantOffset = parent->getNextDescendantOffset(tmpNodePtr->fileOffset);
        if (nextDescendantOffset) {
            node = nullptr; // unload old node to release memory
            auto nextNode = tree->readNode(*nextDescendantOffset);
            nextNode->parent = parent;
            while (nextNode->nodeType() != NodeType::LEAF) {
                auto innerNode = std::dynamic_pointer_cast<AInnerNode>(nextNode);
                nextNode = tree->readNode(*innerNode->descendants[0]);
                nextNode->parent = innerNode;
            }
            node = std::dynamic_pointer_cast<ALeafNode>(nextNode);
            i = 0;
            return;
        }
        tmpNodePtr = parent;
    }

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::dec() -> void {

    if (afterEnd) {
        afterEnd = false;
        return;
    }
// prev record is in the same node
    if (i > 0 && this->node->keys[i - 1] != std::nullopt) {
        i--;
        return;
    }

// if no parent then beforeBegin iterator
    if (node->parent == nullptr) {
        beforeBegin = true;
        return;
    }
    std::shared_ptr<ANode> tmpNodePtr;
    tmpNodePtr = this->node;


// go up and search first left neighbour, if so get most left node
    while (true) {
        if (tmpNodePtr->parent == nullptr) {
            beforeBegin = true;
            return;
        }

        auto parent = std::dynamic_pointer_cast<AInnerNode>(tmpNodePtr->parent);
        auto prevDescendantOffset = parent->getPrevDescendantOffset(tmpNodePtr->fileOffset);
        if (prevDescendantOffset) {
            node = nullptr; // unload old node to release memory
            auto nextNode = tree->readNode(*prevDescendantOffset);
            nextNode->
                    parent = parent;
            while (nextNode->
                    nodeType()
                   != NodeType::LEAF) {
                auto innerNode = std::dynamic_pointer_cast<AInnerNode>(nextNode);
                nextNode = tree->readNode(innerNode->getLastDescendantOffset());
                nextNode->
                        parent = innerNode;
            }
            node = std::dynamic_pointer_cast<ALeafNode>(nextNode);
            i = node->getLastRecordIndex();
            return;
        }
        tmpNodePtr = parent;
    }

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::operator*() -> std::pair<TKey, TValue> {
    if (afterEnd)
        throw std::out_of_range("Tree iterator out of range: afterEnd");
    if (beforeBegin)
        throw std::out_of_range("Tree iterator out of range: beforeBegin");
    auto key = this->node->keys[i];
    auto val = this->node->values[i];
    if (!key || !val) {
        throw std::runtime_error("Internal DB error: next (key, value) not found");
    }
    return std::pair(*key, *val);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::operator==(
        BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator const &other) const -> bool {
    if (this->afterEnd && other.afterEnd) // if both afterEnd
        return true;
    if (this->beforeBegin && other.beforeBegin)
        return true;
    if (this->afterEnd != other.afterEnd) // if different afterEnd
        return false;
    if (this->beforeBegin != other.beforeBegin)
        return false;
    if (this->node == other.node && this->i == other.i)
        return true;
    return false;

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::BPlusTree(fs::path filePath, OpenMode openMode)
        : filePath(std::move(filePath)), configHeader() {

    Tools::debug([] { std::clog << "L: " << ALeafNode::BytesSize() << " I: " << AInnerNode::BytesSize() << '\n'; });
    ANode::ResetCounters();
    switch (openMode) {
        case OpenMode::USE_EXISTING:
            if (!fs::is_regular_file(this->filePath))
                throw std::runtime_error("Couldn't open file: " + fs::absolute(this->filePath).string() + '\n');
            Tools::debug([this] { std::clog << "Opening file: " << fs::absolute(this->filePath) << '\n'; });
            this->file = File(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::ate,
                              [this] { this->incrementReadOperationsCounters(); },
                              [this] { this->incrementWriteOperationsCounters(); });
            if (this->file.bad())
                throw std::runtime_error("Couldn't open file: " + fs::absolute(this->filePath).string() + '\n');
            this->configHeader = this->file.template read<ConfigHeader>(0);
            if (configHeader.leafNodeDegree != TLeafNodeDegree || configHeader.innerNodeDegree != TInnerNodeDegree) {
                throw std::runtime_error("Degrees of nodes are incorrect for current program.\n"s +
                                         "Used by program: <"s + std::to_string(TInnerNodeDegree) +
                                         ", " + std::to_string(TLeafNodeDegree) + ">\nIn File: <" +
                                         std::to_string(configHeader.innerNodeDegree) + ", " +
                                         std::to_string(configHeader.leafNodeDegree) + ">");
            }
            this->root = BPlusTree::readNode(configHeader.rootOffset);
            break;

        case OpenMode::CREATE_NEW:
            this->file = File(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc,
                              [this] { this->incrementReadOperationsCounters(); },
                              [this] { this->incrementWriteOperationsCounters(); });
            if (!this->file.good())
                throw std::runtime_error("Error creating file: " + fs::absolute(this->filePath).string());
            Tools::debug([this] { std::clog << "Creating new db file: " << fs::absolute(this->filePath) << '\n'; });
            this->root = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->file);
            this->updateConfigHeader();

            break;
    }
    Tools::debug([this] { std::clog << "Root: " << *this->root << '\n'; }, 3);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::~BPlusTree() {
    Tools::debug([this] { std::clog << "Closing db file:" << fs::absolute(this->filePath) << '\n'; });
    this->updateConfigHeader();
}


/**
 * Reads node at specified offset and loads it
 * @param fileOffset
 * @return pointer to read and loaded node
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::readNode(size_t fileOffset) -> std::shared_ptr<ANode> {
    char header;
    auto readData = this->file.read(fileOffset,
                                    sizeof(header) + std::max(AInnerNode::BytesSize(), ALeafNode::BytesSize()));
    this->file.clear(); // since we read max of both nodes, we can go eof
    header = readData[0];
    readData.erase(readData.begin());
    if (std::bitset<8>(header)[0] == true) // if node is empty
        throw std::runtime_error("Tried to read empty node at: " + std::to_string(fileOffset));
    std::shared_ptr<ANode> result = nullptr;
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::INNER)) // check node type
        result = std::make_shared<AInnerNode>(fileOffset, this->file);
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
        result = std::make_shared<ALeafNode>(fileOffset, this->file);
    result->load(readData);
    return result;
}


// TODO: use some index to make it work faster
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::AllocateDiskMemory(NodeType nodeType) -> size_t {
    std::fpos<mbstate_t> result;
    auto currentOffset = sizeof(configHeader);
    while (true) {
        char nodeHeader = this->file.template read<char>(currentOffset);

        // if end of file
        if (this->file.eof()) {
            result = currentOffset;
            break;
        }

        // if found space is empty type is good
        if (std::bitset<8>(nodeHeader)[1] == static_cast<int>(nodeType) && std::bitset<8>(nodeHeader)[0] == true) {
            result = currentOffset;
            break;
        }

        // not empty -> search next
        auto stepSize = (std::bitset<8>(nodeHeader)[1] == static_cast<int>(NodeType::LEAF))
                        ? ALeafNode::BytesSize()
                        : AInnerNode::BytesSize();
        currentOffset += stepSize + 1; // plus node header
    }
    if (result < 0) throw std::runtime_error("Unable to allocate disk memory");
    auto offset = static_cast<size_t>(result);

    // Mark space as occupied by simply creating and unloading node
    if (nodeType == NodeType::LEAF) ALeafNode(offset, this->file).markChanged();
    else AInnerNode(offset, this->file).markChanged();
    return offset;
}


/**
 * Tries to compensate node with neighbour
 * @param node node which needs compensation
 * @param key ptr to key added to node (while creating new record only, otherwise nullptr)
 * @param value ptr value added to node (while creating new record and with leaf nodes only, otherwise nullptr)
 * @param nodeOffset descendant added to node (only used with inner nodes)
 * @return true if succeeded and false if failed
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::tryCompensateAndAdd(std::shared_ptr<ANode> node,
                                                                                     TKey const *const key,
                                                                                     TValue const *const value,
                                                                                     size_t nodeOffset) -> bool {
    if (!node) throw std::invalid_argument("Given node argument is nullptr");

    // if node is root -> can't compensate
    if (node->parent == nullptr) return false;
    // determine which node is left / right
    std::shared_ptr<ANode> left, right;
    {
        auto[l, r] = getNodeNeighbours(node);
        if (key) {     // compensation while adding new key
            if (l && !l->full()) {
                left = l;
                right = node;
            } else if (r && !r->full()) {
                left = node;
                right = r;
            } else {
                return false; // if no unfilled neighbours -> can't compensate

            }
        } else { // compensation after deletion
            if (l && (l->fillKeysSize() + node->fillKeysSize() >= 2 * node->degree())) {
                left = l;
                right = node;
            } else if (r && (r->fillKeysSize() + node->fillKeysSize() >= 2 * node->degree())) {
                left = node;
                right = r;
            } else {
                return false; // no nodes which meeting conditions
            }

        }
    }

    // compensate nodes
    auto middleKey = left->compensateWithAndReturnMiddleKey(right, key, value, nodeOffset);
    // update parent with new middle key (biggest key in left node also)
    std::dynamic_pointer_cast<AInnerNode>(node->parent)->setKeyBetweenPtrs(left->fileOffset, right->fileOffset,
                                                                           middleKey);
    return true;
}


/**
 * Splits node and update anncestors recursively
 * @param node node to split
 * @param key
 * @param value used only for leaf nodes
 * @param addedNodeOffset used only for inner nodes
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::splitAndAddRecord(std::shared_ptr<ANode> node,
                                                                                   TKey const &key,
                                                                                   TValue const &value,
                                                                                   size_t addedNodeOffset) -> void {
    // Create new node
    std::shared_ptr<ANode> newNode = nullptr;
    if (node->nodeType() == NodeType::LEAF)
        newNode = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->file);
    else
        newNode = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->file);

    // if root then create new parent (new root)
    if (node == root) {
        auto newRoot = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->file);
        // compensate old root with newly created empty node
        auto midKey = node->compensateWithAndReturnMiddleKey(newNode, &key, &value, addedNodeOffset);
        // add pointers of old root and newly created node to new root
        newRoot->descendants[0] = node->fileOffset;
        newRoot->keys[0] = midKey;
        newRoot->descendants[1] = newNode->fileOffset;
        newRoot->markChanged();
        this->root = newRoot;
        this->updateConfigHeader();
        return;
    }

    // if no parent -> error
    if (node->parent == nullptr) throw std::runtime_error("Internal database error: nullptr node parent");

    // compensate node with newly created node
    auto middleKey = node->compensateWithAndReturnMiddleKey(newNode, &key, &value, addedNodeOffset);

    // add info about this nodes to parent
    auto newNodeOffset = newNode->fileOffset;

    newNode = nullptr; // unload new node, it is needed no more

    // if parent not full -> simply add new key and ptr to new node
    if (!node->parent->full()) {
        std::dynamic_pointer_cast<AInnerNode>(node->parent)->add(middleKey, newNodeOffset);
        return;
    }

    // else try compensate and add
    bool compensationSucceeded = tryCompensateAndAdd(node->parent, &middleKey, &value, newNodeOffset);
    if (compensationSucceeded) return;

    // else split parent
    splitAndAddRecord(node->parent, middleKey, value, newNodeOffset);
}


/**
 * Merges given node with neighbour and updates ancestors recursively
 * @param node node needed to be merged
 * @param key key from leaf which need to be replaced in ancestors
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::merge(std::shared_ptr<BPlusTree::ANode> node,
                                                                       TKey const *const key) -> void {
    // get neighbour to merge with
    // this key we will need to replace with new greatest key in ancestors
    TKey oldKey;
    if (node->nodeType() == NodeType::LEAF) {
        oldKey = *std::dynamic_pointer_cast<ALeafNode>(node)->getLastKey();
    }
    auto[left, right] = this->getNodeNeighbours(node);

    if (left) {
        right = nullptr;
        // get max key of left descendant
        auto key = std::dynamic_pointer_cast<AInnerNode>(left->parent)->getKeyBetweenPtrs(left->fileOffset,
                                                                                          node->fileOffset);
        left->mergeWith(node, &key);
        node = left;
        left = nullptr;
    } else if (right) {
        left = nullptr;
        // max key is from argument
        node->mergeWith(right, key);
        right = nullptr;
    } else {
        throw std::runtime_error("Internal error: merge: no selectedNeighbour");
    }

    // update parent key with new last key
    TKey lastKey;
    if (node->nodeType() == NodeType::LEAF) {
        auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);
        lastKey = *leafNode->getLastKey();
        std::shared_ptr<AInnerNode> parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
        while (parent != nullptr) {
            if (parent->contains(oldKey)) {
                parent->swapKeys(oldKey, lastKey);
            }
            parent = std::dynamic_pointer_cast<AInnerNode>(parent->parent);
        }
    }

    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    // remove out-of-date descendant and key
    auto nodeState = parent->removeKeyOffsetAfter(node->fileOffset);

    // parent node is valid
    if (nodeState == NodeState::OK)
        return;

    // if parent is root
    if (parent == root) {
        // if root contains 0 items -> remove and make new root from descendant
        if (parent->getEntries().first.empty()) {
            root->markEmpty();
            root = node;
        } // else do nothing
        return;
    }

    // parent node is too small
    if (nodeState & NodeState::TOO_SMALL) {
        // try compensate with neighbour
        bool compensationSuccess = tryCompensateAndAdd(parent);
        if (!compensationSuccess) {
            merge(parent, &lastKey);
        }
    }


}


/**
 * Returns pair of ptrs to left and right neighbour
 * @param node node which neighbours are looked for
 * @return pair of ptrs to loaded neighbour nodes, nullptr if no such node
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getNodeNeighbours(std::shared_ptr<ANode> node)
-> std::pair<std::shared_ptr<ANode>, std::shared_ptr<ANode>> {

    auto result = std::make_pair<std::shared_ptr<ANode>, std::shared_ptr<ANode>>(nullptr, nullptr);
    if (node->parent == nullptr) return result;
    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    auto lOffset = parent->getPrevDescendantOffset(node->fileOffset);
    auto rOffset = parent->getNextDescendantOffset(node->fileOffset);

    // left neighbour found
    if (lOffset) {
        result.first = BPlusTree::readNode(*lOffset);
        result.first->parent = node->parent;
    }

    // right neighbour found
    if (rOffset) {
        result.second = BPlusTree::readNode(*rOffset);
        result.second->parent = node->parent;
    }

    return result;
}


/**
 * Updates config header in db file
 * @return
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::updateConfigHeader() -> void {
    configHeader.rootOffset = this->root->fileOffset;
    configHeader.innerNodeDegree = TInnerNodeDegree;
    configHeader.leafNodeDegree = TLeafNodeDegree;
    this->file.write(0, configHeader);
}


/**
 * Creates new records with given key and value
 * @param key
 * @param value
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::createRecord(TKey const &key, TValue const &value) -> void {
    // find leaf to insert record into
    auto leafNode = this->findProperLeaf(key);

    // if key exists then Exit
    if (leafNode->contains(key)) {
        std::cout << "Given key already exists. Record not added.\n";
        return;
    }

    // if node not full -> insert record
    if (!leafNode->full()) {
        leafNode->insert(key, value);
        return;
    }

    // else try compensate node and add record
    bool compensationSucceeded = tryCompensateAndAdd(leafNode, &key, &value);
    if (compensationSucceeded) return;

    // else split node and add record
    splitAndAddRecord(leafNode, key, value);
}


/**
 * Reads and returns record with given key
 * @param key
 * @return optional record, nullopt if doesn't exist
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::readRecord(TKey const &key) -> std::optional<TValue> {
    return findProperLeaf(key)->readRecord(key);
}


/**
 * Updates record with given key, throws if key not found
 * @param key key of record to update
 * @param value data to update to
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::updateRecord(TKey const &key, TValue const &value) -> void {
    this->findProperLeaf(key)->updateRecord(key, value);
}


/**
 * Deletes record with given key, does nothing if key doesn't exist
 * @param key key of record to delete
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::deleteRecord(TKey const &key) -> void {
    // find ndoe possibly containing record
    auto node = this->findProperLeaf(key);
    if (!node->contains(key)) {
        throw std::runtime_error("Key " + std::to_string(key) + " doesn't exist");
    }

    // remove
    auto nodeState = node->deleteRecord(key);
    // if root -> no need to do anything
    if (node == root) return;
    // node is ok after deletion
    if (nodeState == OK)
        return;

    // if deleted last key get new last key and put it in the ancestor instead of old one (if exists)
    if (nodeState & NodeState::DELETED_LAST) {
        auto lastKey = node->getLastKey();
        if (!lastKey)
            throw std::runtime_error("Internal error: Unable to determine new greatest key in node: " +
                                     std::to_string(node->fileOffset));
        auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
        while (parent != nullptr) {
            if (parent->contains(key)) {
                parent->swapKeys(key, *lastKey);
            }
            parent = std::dynamic_pointer_cast<AInnerNode>(parent->parent);
        }
    }

    if (nodeState & NodeState::TOO_SMALL) {
        // try compensate with neighbour
        bool compensationSuccess = tryCompensateAndAdd(node);
        if (!compensationSuccess) {
            merge(node);
        }
    }
}

/**
 * Finds offset of descendant which possibly contains given key
 * @param node searched node
 * @param key
 * @return
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::findProperDescendantOffset(std::shared_ptr<ANode> node,
                                                                                       TKey const &key) -> NodeOffset {
    auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
    auto offset = innerNode->getDescendantsOfKey(key).first;
    return offset;
}


/**
 * Returns ptr loaded Leaf probably containing given key
 * @param key
 * @return ptr to leaf probably containing given key
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::findProperLeaf(TKey const &key)
-> std::shared_ptr<ALeafNode> {
    std::shared_ptr<ANode> node = root;
    while (node->nodeType() != NodeType::LEAF) {
        auto descendantOffset = findProperDescendantOffset(node, key);
        auto nextNode = readNode(descendantOffset);
        nextNode->parent = node;
        node = nextNode;
    }
    return std::dynamic_pointer_cast<ALeafNode>(node);
}

/**
 * Get name of tree
 * @return string with name
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::name() const -> std::string {
    return "B+ tree with keys="s + Tools::typeName<TKey>() + ", values=" + Tools::typeName<TValue>();
}


/**
 * Callback for File class
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::incrementWriteOperationsCounters() {
    if (!countersEnabled) return;
    sessionDiskWritesCount++;
    currentOperationDiskWritesCount++;
}


/**
 * Callback for File class
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::incrementReadOperationsCounters() {
    if (!countersEnabled) return;
    sessionDiskReadsCount++;
    currentOperationDiskReadsCount++;
}


/**
 * Resets disk IO counters and max nodes in memory counter
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::resetCounters() -> void {
    sessionDiskReadsCount
            = sessionDiskWritesCount
            = currentOperationDiskReadsCount
            = currentOperationDiskWritesCount = 0;
    ANode::ResetCounters();
}


/**
 * Prints all tree nodes on stdout
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::print() -> void {
    this->root->unload();
    this->printNodeAndDescendants(this->root);
}


/**
 * Prints node and descendants on stdout
 * @param node
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printNodeAndDescendants(std::shared_ptr<ANode> node)
-> void {
    std::cout << *node << ' ';
    std::cout.flush();
    if (node->nodeType() == NodeType::INNER) {
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        for (auto &descOffset : innerNode->getEntries().second) {
            printNodeAndDescendants(BPlusTree::readNode(descOffset));
            std::cout << ' ';
            std::cout.flush();
        }
    }
}


/**
 * Draws tree using Graphviz
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::draw() -> void {
    this->root->unload();
    // TODO: remove tmp file
    GVC_t *gvc;
    Agraph_t *g;
    gvc = gvContext();
    g = agmemread(this->gvcPrintTree().str().c_str());
    std::string filePath = tmpnam(nullptr);
    auto file = fopen(filePath.c_str(), "w");
    gvLayout(gvc, g, "dot");
    gvRender(gvc, g, "svg", file);
    gvFreeLayout(gvc, g);
    agclose(g);
    system(("xdg-open " + filePath /*+ " >>/dev/null 2>>/dev/null"*/).c_str());
    fclose(file);
    gvFreeContext(gvc);
}

/**
 * @return stringstream containing B+Tree in dot format
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::gvcPrintTree() -> std::stringstream {
    std::stringstream ss;
    ss << "digraph g{node [ shape = record,height=.1];";
    gvcPrintNodeAndDescendants(this->root, ss);
    ss << "}";
    Tools::debug([&] { std::clog << ss.str() << '\n'; }, 1);
    return ss;
}


/**
 * Function for constructing B+Tree in dot format
 * @param node node to draw
 * @param ss reference to stringstream
 * @return reference to stringstream
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::gvcPrintNodeAndDescendants(
        std::shared_ptr<BPlusTree::ANode> node, std::stringstream &ss) -> std::stringstream & {
    ss << *node;
    if (node->nodeType() == NodeType::INNER) {
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        for (auto &descOffset : innerNode->getEntries().second) {
            gvcPrintNodeAndDescendants(BPlusTree::readNode(descOffset), ss);
        }
    }
    return ss;
}


/**
 * Prints db file to stdout in human readable form
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printFile() -> void {
    this->unload();
    auto offset = 0u;
    auto configHeader = file.read<ConfigHeader>(offset);
    std::cout << "0:\tConfigHeader {rootOffset: " << configHeader.rootOffset
              << ", innerNodeDegree: " << configHeader.innerNodeDegree
              << ", leafNodeDegree: " << configHeader.leafNodeDegree
              << "}";
    offset += sizeof(ConfigHeader);
    while (true) {
        std::cout << "\n";
        auto nodeHeader = file.read<char>(offset);
        if (file.eof()) break;
        // if found space is empty and big enough
        std::cout << offset << ":\theader, ";
        if (std::bitset<8>(nodeHeader)[0] == true) {
            if (std::bitset<8>(nodeHeader)[1] == static_cast<int>(NodeType::LEAF)) {
                std::cout << "LNode: ";
            } else {
                std::cout << "INode: ";
            }
            std::cout << "empty ";

        } else {
            readNode(offset)->print(std::cout);
        }

        auto stepSize = (std::bitset<8>(nodeHeader)[1] == static_cast<int>(NodeType::LEAF))
                        ? ALeafNode::BytesSize()
                        : AInnerNode::BytesSize();
        offset += stepSize + 1;
    }
    file.clear();
}


/**
 * @return integer containing high of the tree
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getHeight() -> uint64_t {
    auto node = root;
    uint64_t counter = 1;
    while (node->nodeType() != NodeType::LEAF) {
        node = readNode(*std::dynamic_pointer_cast<AInnerNode>(node)->descendants[0]);
        counter++;
    }
    return counter;
}


/**
 * @return pair of (inner nodes count, leaf nodes count)
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getNodesCount() -> std::pair<uint64_t, uint64_t> {
    auto counters = std::pair<uint64_t, uint64_t>(0, 0);
    this->getNodesCount(this->root, counters);
    return counters;
}


/**
 * Recursive function for nodes counting
 * @param node node to count and find descendants
 * @param counters reference to counters
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getNodesCount(std::shared_ptr<ANode> node,
                                                                          std::pair<uint64_t, uint64_t> &counters)
-> void {

    if (node->nodeType() == NodeType::LEAF) {
        counters.second++;
        return;
    }
    counters.first++;
    auto &descendants = std::dynamic_pointer_cast<AInnerNode>(node)->descendants;
    for (auto &descendant:descendants) {
        if (!descendant) break;
        this->getNodesCount(this->readNode(*descendant), counters);
    }
}


/**
 * @return ptr to loaded first (contaning smallest keys) leaf
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ALeafNode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getFirstLeaf() {
    auto node = this->root;
    if (node == nullptr) {
        throw std::runtime_error("Root is nullptr");
    }
    while (node->nodeType() != NodeType::LEAF) {
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        auto offset = innerNode->descendants[0];
        if (!offset) {
            throw std::runtime_error("Unable to find descendant");
        }
        node = readNode(*offset);
        node->parent = innerNode;
    }
    return std::dynamic_pointer_cast<ALeafNode>(node);
}


/**
 * @return ptr to loaded last (contaning greatest keys) leaf
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ALeafNode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getLastLeaf() {
    auto node = this->root;
    if (node == nullptr) {
        throw std::runtime_error("Root is nullptr");
    }
    while (node->nodeType() != NodeType::LEAF) {
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        auto offset = innerNode->getLastDescendantOffset();
        node = readNode(offset);
        node->parent = innerNode;
    }
    return std::dynamic_pointer_cast<ALeafNode>(node);
}


/**
 * Counts and returns records number
 * @return count of records
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
auto BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getRecordsNumber() -> uint64_t {
    uint64_t i = 0;
    for (auto const &_:*this)++i;
    return i;
}

#endif //SBD2_B_PLUS_TREE_HH