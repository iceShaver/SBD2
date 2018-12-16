
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


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree;


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::ostream &operator<<(std::ostream &, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &);


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree final {
    struct ConfigHeader {
        uint64_t rootOffset;
        uint64_t innerNodeDegree;
        uint64_t leafNodeDegree;
    };

    class Iterator;

    using ANode = Node<TKey, TValue>;
    using AInnerNode = InnerNode<TKey, TValue, TInnerNodeDegree>;
    using ALeafNode = LeafNode<TKey, TValue, TLeafNodeDegree>;

public:
    BPlusTree() = delete;
    BPlusTree(BPlusTree &&) = delete;
    BPlusTree(BPlusTree const &) = delete;
    BPlusTree &operator=(BPlusTree &&) = delete;
    BPlusTree &operator=(BPlusTree const &) = delete;
    ~BPlusTree();

    explicit BPlusTree(fs::path filePath, OpenMode openMode = OpenMode::USE_EXISTING);

    std::shared_ptr<ANode> readNode(size_t fileOffset);
    size_t AllocateDiskMemory(NodeType nodeType);

    // CRUD operations
    void createRecord(TKey const &key, TValue const &value);
    std::optional<TValue> readRecord(TKey const &key);
    void updateRecord(TKey const &key, TValue const &value);
    void deleteRecord(TKey const &key);

    size_t findProperDescendantOffset(std::shared_ptr<ANode> node, TKey const &key);
    std::shared_ptr<ALeafNode> findProperLeaf(TKey const &key);

    bool tryCompensateAndAdd(std::shared_ptr<ANode> node,
                             TKey const &key, TValue const &value, size_t nodeOffset = 0);
    void splitAndAddRecord(std::shared_ptr<ANode> node,
                           TKey const &key, TValue const &value, size_t addedNodeOffset = 0);
    void mergeAndRemove(std::shared_ptr<ANode> node, TKey const &key); // TODO: check if nodeOffset is necessary
    std::shared_ptr<ANode> getNodeUnfilledNeighbour(std::shared_ptr<ANode> node);
    BPlusTree &print();
    void printFile();
    std::stringstream gvcPrintTree();
    void printNodeAndDescendants(std::shared_ptr<ANode> node);
    std::stringstream &gvcPrintNodeAndDescendants(std::shared_ptr<ANode> node, std::stringstream &ss);
    auto getAllocationsCounter() const { return this->allocationsCounter; }
    friend Iterator;
    friend Dbms;
    friend std::ostream &operator<<<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>(
            std::ostream &os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree);
    void draw();
    void truncate();
    void unload();
    std::string name() const;
    constexpr auto innerNodeDegree() const { return TInnerNodeDegree; }
    constexpr auto leafNodeDegree() const { return TLeafNodeDegree; }
    auto getSessionDiskReadsCout() const { return sessionDiskReadsCount; }
    auto getSessionDiskWritesCount() const { return sessionDiskWritesCount; }
    auto getCurrentOperationDiskReadsCount() const { return currentOperationDiskReadsCount; }
    auto getCurrentOperationDiskWritesCount() const { return currentOperationDiskWritesCount; }

    Iterator const begin();
    Iterator const end() { return Iterator(); }
private:
    void beginOperation();
    void incrementWriteOperationsCounters();
    void incrementReadOperationsCounters();
    void resetCounters();


    uint64_t sessionDiskReadsCount = 0;
    uint64_t sessionDiskWritesCount = 0;
    uint64_t currentOperationDiskReadsCount = 0;
    uint64_t currentOperationDiskWritesCount = 0;

    fs::path filePath;
    //std::fstream *fileHandle;
    File file;
    std::shared_ptr<ANode> root;
    ConfigHeader configHeader;
    BPlusTree &updateConfigHeader();
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator {
    // types required for STL
    using difference_type = long long;
    using value_type = std::pair<TKey, TValue>;
    using pointer = const std::pair<TKey, TValue> *;
    using reference = std::pair<TKey, TValue> &;
    using iterator_category = std::bidirectional_iterator_tag;
    friend BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>;
public:
    Iterator &operator++();     // ++x;
    Iterator const operator++(int); // x++
    value_type operator*();
    bool operator==(Iterator const &other) const;
    bool operator!=(Iterator const &other) const { return !(*this == other); }
private:
    Iterator(std::shared_ptr<ALeafNode> node,
             BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree) : end(false),
                                                                                 node(std::move(node)),
                                                                                 i(0),
                                                                                 tree(std::move(
                                                                                         tree)) {}
    Iterator() : end(true), node(nullptr), i(0), tree(nullptr) {}

    bool end;
    std::shared_ptr<ALeafNode> node;
    size_t i;
    BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> *tree;
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::operator++() {
    // next record is in the same node
    if (i < this->node->keys.size() - 1 && this->node->keys[i + 1] != std::nullopt) {
        i++;
        return *this;
    }

    // if no parent then end iterator
    if (node->parent == nullptr) {
        end = true;
        return *this;
    }
    std::shared_ptr<ANode> tmpNodePtr;
    tmpNodePtr = this->node;


    // go up and search first right neighbour, if so get most left node
    while (true) {
        if (tmpNodePtr->parent == nullptr) {
            end = true;
            return *this;
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
            return *this;
        }
        tmpNodePtr = parent;
    }

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator const
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::operator++(int) {
    auto const tmp = *this;
    this->operator++();
    return tmp;
}
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::pair<TKey, TValue>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::operator*() {
    if (end)
        throw std::out_of_range("Tree iterator out of range: end");
    auto key = this->node->keys[i];
    auto val = this->node->values[i];
    if (!key || !val) {
        throw std::runtime_error("Internal DB error: next (key, value) not found");
    }
    return std::pair(*key, *val);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
bool
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator::operator==(
        BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator const &other) const {
    if (this->end && other.end) // if both end
        return true;
    if (this->end != other.end) // if different end
        return false;
    if (this->node == other.node && this->i == other.i)
        return true;
    return false;

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::BPlusTree(fs::path filePath, OpenMode openMode)
        : filePath(std::move(filePath)), configHeader() {
    Tools::debug([] { std::clog << "L: " << ALeafNode::BytesSize() << " I: " << AInnerNode::BytesSize() << '\n'; });
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
            if (configHeader.leafNodeDegree != TLeafNodeDegree
                || configHeader.innerNodeDegree != TInnerNodeDegree) {
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
 * @tparam TKey
 * @tparam TValue
 * @tparam TInnerNodeDegree
 * @tparam TLeafNodeDegree
 * @param fileOffset
 * @return pointer to read and loaded node
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::readNode(size_t fileOffset) {
    char header;
    auto readData = this->file.read(fileOffset,
                                    sizeof(header) + std::max(AInnerNode::BytesSize(), ALeafNode::BytesSize()));
    this->file.clear(); // since we read max of both nodes, we can go eof
    header = readData[0];
    readData.erase(readData.begin());
    if (std::bitset<8>(header)[0] == true) throw std::runtime_error("Tried to read empty node!");
    std::shared_ptr<ANode> result = nullptr;
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::INNER))
        result = std::make_shared<AInnerNode>(fileOffset, this->file);
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
        result = std::make_shared<ALeafNode>(fileOffset, this->file);
    result->load(readData);
    return result;
}


// TODO: use some index to make it work faster
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
size_t
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::AllocateDiskMemory(NodeType nodeType) {
    std::fpos<mbstate_t> result;
    auto currentOffset = sizeof(configHeader);
    while (true) {
        char nodeHeader = this->file.template read<char>(currentOffset);

        // if end of file
        if (this->file.eof()) {
            result = currentOffset;
            break;
        }

        // if found space is empty and big enough
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

    // This is only for purpose of mark space as occupied, TODO: do something more efficient e.g.: create header only
    std::bitset<8> newHeader;
    newHeader[1] = static_cast<int>(nodeType);
    newHeader[0] = false; // not empty;
    char headerByte = static_cast<char>(newHeader.to_ulong());
    this->file.write(offset, headerByte);
    // it is for extending the file to needed size, TODO: do something better
    if (nodeType == NodeType::LEAF) ALeafNode(offset, this->file).markChanged();
    else AInnerNode(offset, this->file).markChanged();
    return offset;
}

// TODO: ??make params optional?? or allow node to have more records and compensate/split/ it after adding record
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
bool
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::tryCompensateAndAdd(std::shared_ptr<ANode> node,
                                                                                TKey const &key,
                                                                                TValue const &value,
                                                                                size_t nodeOffset) {
    if (!node) throw std::invalid_argument("Given node argument is nullptr");

    // if node is root -> can't compensate
    if (node->parent == nullptr) return false;

    auto selectedNeighbour = getNodeUnfilledNeighbour(node);

    // if no unfilled neighbours -> can't compensate
    if (!selectedNeighbour) return false;

    auto middleKey = node->compensateWithAndReturnMiddleKey(selectedNeighbour, key, value, nodeOffset);

    // update parent with middle key
    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
    parent->setKeyBetweenPtrs(node->fileOffset, selectedNeighbour->fileOffset, middleKey);
    // compensation succeeded
    return true;
}


/**
 * Returns loaded neighbour if it exists (nullptr if don't, left is considered first)
 * @tparam TKey
 * @tparam TValue
 * @tparam TInnerNodeDegree
 * @tparam TLeafNodeDegree
 * @param node Node which neighbours we want to find, node has to have the parent
 * @return nodeNeighbourPtr
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getNodeUnfilledNeighbour(std::shared_ptr<ANode> node) {
    // no neighbours if no parent
    if (node->parent == nullptr) return nullptr;

    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
    auto currentNodeOffsetIterator = std::find(parent->descendants.begin(), parent->descendants.end(),
                                               node->fileOffset);
    if (currentNodeOffsetIterator == parent->descendants.end())
        throw std::runtime_error("Internal DB error: couldn't find ptr to current node at parent's");
    auto currentNodeOffsetIndex = currentNodeOffsetIterator - parent->descendants.begin();
    auto leftNodeOffset = (currentNodeOffsetIndex > 0)
                          ? parent->descendants[currentNodeOffsetIndex - 1]
                          : std::nullopt;

    // left neighbour found
    if (leftNodeOffset) {
        auto result = BPlusTree::readNode(*leftNodeOffset);
        if (!result->full()) return result;
    }

    auto rightNodeOffset = (currentNodeOffsetIndex < parent->descendants.size() - 1)
                           ? parent->descendants[currentNodeOffsetIndex + 1]
                           : std::nullopt;

    // right neighbour found
    if (rightNodeOffset) {
        auto result = BPlusTree::readNode(*rightNodeOffset);
        if (!result->full()) return result;
    }

    // no unfilled neighbours found
    return nullptr;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::splitAndAddRecord(std::shared_ptr<ANode> node,
                                                                              TKey const &key,
                                                                              TValue const &value,
                                                                              size_t addedNodeOffset) {
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
        auto midKey = node->compensateWithAndReturnMiddleKey(newNode, key, value, addedNodeOffset);
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
    auto middleKey = node->compensateWithAndReturnMiddleKey(newNode, key, value, addedNodeOffset);

    // add info about this nodes to parent
    auto newNodeOffset = newNode->fileOffset;

    newNode = nullptr;
    // if parent not full -> simply add new key and ptr
    if (!node->parent->full()) {
        std::dynamic_pointer_cast<AInnerNode>(node->parent)->add(middleKey, newNodeOffset);
        return;
    }

    // else try compensate and add
    bool compensationSucceeded = tryCompensateAndAdd(node->parent, middleKey, value, newNodeOffset);
    if (compensationSucceeded) return;

    // else split parent
    splitAndAddRecord(node->parent, middleKey, value, newNodeOffset);

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::ostream &
operator<<(std::ostream &os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree) {
    return os << "Printing whole B-Tree:\n" << *bPlusTree.root << '\n'; //TODO: implement printing whole tree
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::updateConfigHeader() {
    configHeader.rootOffset = this->root->fileOffset;
    configHeader.innerNodeDegree = TInnerNodeDegree;
    configHeader.leafNodeDegree = TLeafNodeDegree;
    this->file.write(0, configHeader);
    return *this;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::print() {
    this->root->unload();
    this->printNodeAndDescendants(this->root);
    return *this;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printNodeAndDescendants(std::shared_ptr<ANode> node) {
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


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::createRecord(TKey const &key, TValue const &value) {
    this->beginOperation();
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
    bool compensationSucceeded = tryCompensateAndAdd(leafNode, key, value);
    if (compensationSucceeded) return;

    // else split node and add record
    splitAndAddRecord(leafNode, key, value);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::optional<TValue>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::readRecord(TKey const &key) {
    this->beginOperation();
    return findProperLeaf(key)->readRecord(key);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::updateRecord(TKey const &key, TValue const &value) {
    this->beginOperation();
    this->findProperLeaf(key)->updateRecord(key, value);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::deleteRecord(TKey const &key) {
    this->beginOperation();
    auto node = this->findProperLeaf(key);
    if (!node->contains(key)) {
        std::cout << "Key " << key << "doesn't exist\n";
        return;
    }

    // remove
    // if node size is OK -> return

    // else try compensate with neighbour

    // else merge with neighbour (sum of elements of node and neighbour has to be <TLeafNodeDegree, 2*TLeafNodeDegree>
    // if parent has then less than TInnerNodeDegree keys -> try compensate parent, if not possible -> merge

    // done
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
size_t
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::findProperDescendantOffset(std::shared_ptr<ANode> node,
                                                                                       TKey const &key) {
    if (node->nodeType() == NodeType::LEAF)
        throw std::invalid_argument("Can't find descendants of leaf node");
    auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
    auto foundLowerBound = std::lower_bound(innerNode->keys.begin(), innerNode->keys.end(), key,
                                            [](auto element, auto value) {
                                                if (!element) return false;
                                                return *element < value;
                                            });
    auto ptrIndex = foundLowerBound - innerNode->keys.begin();
    auto descendantOffset = innerNode->descendants[ptrIndex];
    if (!descendantOffset)
        throw std::runtime_error("Internal DB error: couldn't find descendant");
    return *descendantOffset;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::draw() {
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


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::stringstream
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::gvcPrintTree() {
    std::stringstream ss;
    ss << "digraph g{node [ shape = record,height=.1];";
    gvcPrintNodeAndDescendants(this->root, ss);
    ss << "}";
    Tools::debug([&] { std::clog << ss.str() << '\n'; }, 1);
    return ss;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::stringstream &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::gvcPrintNodeAndDescendants(
        std::shared_ptr<BPlusTree::ANode> node, std::stringstream &ss) {
    ss << *node;
    if (node->nodeType() == NodeType::INNER) {
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        for (auto &descOffset : innerNode->getEntries().second) {
            gvcPrintNodeAndDescendants(BPlusTree::readNode(descOffset), ss);
        }
    }
    return ss;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ALeafNode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::findProperLeaf(TKey const &key) {
    std::shared_ptr<ANode> node = root;
    while (node->nodeType() != NodeType::LEAF) {
        auto descendantOffset = findProperDescendantOffset(node, key);
        auto nextNode = readNode(descendantOffset);
        nextNode->parent = node;
        node = nextNode;
    }
    return std::dynamic_pointer_cast<ALeafNode>(node);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::mergeAndRemove(std::shared_ptr<BPlusTree::ANode> node,
                                                                                TKey const &key) {

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::truncate() {
    //*this = BPlusTree(this->filePath, OpenMode::CREATE_NEW);// TODO: Test this;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::string BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::name() const {
    return "B+ tree with keys="s + Tools::typeName<TKey>() + ", values=" + Tools::typeName<TValue>();
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::beginOperation() {
    currentOperationDiskWritesCount = currentOperationDiskReadsCount = 0;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::incrementWriteOperationsCounters() {
    sessionDiskWritesCount++;
    currentOperationDiskWritesCount++;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::incrementReadOperationsCounters() {
    sessionDiskReadsCount++;
    currentOperationDiskReadsCount++;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::resetCounters() {
    sessionDiskReadsCount = sessionDiskWritesCount = currentOperationDiskReadsCount = currentOperationDiskWritesCount = 0;
    ANode::ResetCounters();
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::unload() {
    this->root->unload();
    this->updateConfigHeader();
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printFile() {
    this->unload();
    auto offset = 0u;
    auto configHeader = file.read<ConfigHeader>(offset);
    std::cout << "0:\tConfigHeader {rootOffset: " << configHeader.rootOffset
              << ", innerNodeDegree: " << configHeader.innerNodeDegree
              << ", leafNodeDegree: " << configHeader.leafNodeDegree;
    offset += sizeof(ConfigHeader);
    while (true) {
        std::cout << "\n";
        auto nodeHeader = file.read<char>(offset);
        if (file.eof()) break;
        // if found space is empty and big enough
        std::cout << offset << ":\theader, ";
        if (std::bitset<8>(nodeHeader)[0] == true) {
            std::cout << "empty ";
            if (std::bitset<8>(nodeHeader)[1] == static_cast<int>(NodeType::LEAF)) {
                std::cout << "leaf node ";
            } else {
                std::cout << "inner node ";
            }
            continue;
        }
        readNode(offset)->print(std::cout);

        // not empty -> search next
        auto stepSize = (std::bitset<8>(nodeHeader)[1] == static_cast<int>(NodeType::LEAF))
                        ? ALeafNode::BytesSize()
                        : AInnerNode::BytesSize();
        offset += stepSize + 1;
    }
    file.clear();
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::Iterator const
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::begin() {
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
    return Iterator(std::dynamic_pointer_cast<ALeafNode>(node), this);
}


#endif //SBD2_B_PLUS_TREE_HH