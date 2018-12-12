
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

using namespace std::string_literals;
namespace fs = std::filesystem;

class Dbms;

enum class OpenMode { USE_EXISTING, CREATE_NEW };


struct ConfigHeader {
    uint64_t rootOffset;
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree;


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::ostream &operator<<(std::ostream &, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &);


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
class BPlusTree final {

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
    std::stringstream gvcPrintTree();
    void printNodeAndDescendants(std::shared_ptr<ANode> node);
    std::stringstream &gvcPrintNodeAndDescendants(std::shared_ptr<ANode> node, std::stringstream &ss);
    auto getAllocationsCounter() const { return this->allocationsCounter; }
    friend Dbms;
    friend std::ostream &operator<<<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>(
            std::ostream &os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree);
    void draw();
    void truncate();
    std::string name() const;
    constexpr auto innerNodeDegree() const { return TInnerNodeDegree; }
    constexpr auto leafNodeDegree() const { return TLeafNodeDegree; }
    auto getSessionDiskReadsCout() const { return sessionDiskReadsCount; }
    auto getSessionDiskWritesCount() const { return sessionDiskWritesCount; }
    auto getCurrentOperationDiskReadsCount() const { return currentOperationDiskReadsCount; }
    auto getCurrentOperationDiskWritesCount() const { return currentOperationDiskWritesCount; }
private:
    void beginOperation();
    void incrementWriteOperationsCounters();
    void incrementReadOperationsCounters();
    void resetDiskOperationsCounters();
    template<typename TResult> TResult diskRead(size_t offset);
    std::vector<char> diskRead(size_t offset, size_t size);
    template<typename TData>
    void diskWrite(size_t offset, TData const &data);
    void diskWrite(size_t offset, std::vector<char> const &data);


    uint64_t sessionDiskReadsCount = 0;
    uint64_t sessionDiskWritesCount = 0;
    uint64_t currentOperationDiskReadsCount = 0;
    uint64_t currentOperationDiskWritesCount = 0;

    fs::path filePath;
    std::fstream fileHandle;
    std::shared_ptr<ANode> root;
    ConfigHeader configHeader;
    BPlusTree &updateConfigHeader();
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::BPlusTree(fs::path filePath, OpenMode openMode)
        : filePath(std::move(filePath)), configHeader() {
    Tools::debug([] { std::clog << "L: " << ALeafNode::BytesSize() << " I: " << AInnerNode::BytesSize() << '\n'; });
    this->fileHandle.rdbuf()->pubsetbuf(0, 0);
    switch (openMode) {
        case OpenMode::USE_EXISTING:
            if (!fs::is_regular_file(this->filePath))
                throw std::runtime_error("Couldn't open file: " + fs::absolute(this->filePath).string() + '\n');
            Tools::debug([this] { std::clog << "Opening file: " << fs::absolute(this->filePath) << '\n'; });
            this->fileHandle.open(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::ate);
            if (this->fileHandle.bad())
                throw std::runtime_error("Couldn't open file: " + fs::absolute(this->filePath).string() + '\n');
            this->configHeader = this->diskRead<ConfigHeader>(0);
            this->root = BPlusTree::readNode(configHeader.rootOffset);
            break;

        case OpenMode::CREATE_NEW:
            this->fileHandle.open(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
            if (!this->fileHandle.good())
                throw std::runtime_error("Error creating file: " + fs::absolute(this->filePath).string());
            Tools::debug([this] { std::clog << "Creating new db file: " << fs::absolute(this->filePath) << '\n'; });
            this->root = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
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
 * Reads node at specified offset and loads it TODO: change it to read one block of size max(innernodedegree, leafnodedegree)
 * @tparam TKey
 * @tparam TValue
 * @tparam TInnerNodeDegree
 * @tparam TLeafNodeDegree
 * @param fileHandle
 * @param fileOffset
 * @return pointer to read and loaded node
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::readNode(size_t fileOffset) {
    char header;
    auto readData = this->diskRead(fileOffset,
                                   sizeof(header) + std::max(AInnerNode::BytesSize(), ALeafNode::BytesSize()));
    fileHandle.clear(); // since we read max of both nodes, we can go eof
    header = readData[0];
    readData.erase(readData.begin());
    if (std::bitset<8>(header)[0] == true) throw std::runtime_error("Tried to read empty node!");
    std::shared_ptr<ANode> result = nullptr;
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::INNER))
        result = std::make_shared<AInnerNode>(fileOffset, fileHandle);
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
        result = std::make_shared<ALeafNode>(fileOffset, fileHandle);
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
        char nodeHeader = this->diskRead<char>(currentOffset);
        // if end of file
        if (this->fileHandle.tellg() >= fs::file_size(this->filePath)) {
            if (this->fileHandle.eof()) {
                this->fileHandle.clear();
                result = this->fileHandle.tellg();
            } else {
                result = this->fileHandle.tellg() - 1;
            }
            break;
        }

        // if found space is empty and big enough
        if (std::bitset<8>(nodeHeader)[1] == static_cast<int>(nodeType) && std::bitset<8>(nodeHeader)[0] == true) {
            result = this->fileHandle.tellg() - 1;
            break;
        }

        // not empty -> search next
        auto stepSize = (std::bitset<8>(nodeHeader)[1] == static_cast<int>(NodeType::LEAF))
                        ? ALeafNode::BytesSize()
                        : AInnerNode::BytesSize();
        currentOffset += stepSize;
    }
    if (result < 0) throw std::runtime_error("Unable to allocate disk memory");
    auto offset = static_cast<size_t>(result);

    // This is only for purpose of mark space as occupied, TODO: do something more efficient e.g.: create header only
    std::bitset<8> newHeader;
    newHeader[1] = static_cast<int>(nodeType);
    newHeader[0] = false; // not empty;
    char headerByte = static_cast<char>(newHeader.to_ulong());
    this->diskWrite(offset, headerByte);
    // it is for extending the file to needed size, TODO: do something better
    if (nodeType == NodeType::LEAF) ALeafNode(offset, this->fileHandle).markChanged();
    else AInnerNode(offset, this->fileHandle).markChanged();
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
        newNode = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
    else
        newNode = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->fileHandle);

    // if root then create new parent (new root)
    if (node == root) {
        auto newRoot = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
        // compensate old root with newly created empty node
        auto midKey = node->compensateWithAndReturnMiddleKey(newNode, key, value, addedNodeOffset);
        // add pointers of old root and newly created node to new root
        newRoot->descendants[0] = node->fileOffset;
        newRoot->keys[0] = midKey;
        newRoot->descendants[1] = newNode->fileOffset;
        this->root = newRoot;
        this->updateConfigHeader();
        return;
    }

    // if no parent -> error
    if (node->parent == nullptr) throw std::runtime_error("Internal database error: nullptr node parent");

    // compensate node with newly created node

    auto middleKey = node->compensateWithAndReturnMiddleKey(newNode, key, value, addedNodeOffset);

    // add info about this nodes to parent
    //auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    // if parent not full -> simply add new key and ptr
    if (!node->parent->full()) {
        std::dynamic_pointer_cast<AInnerNode>(node->parent)->add(middleKey, newNode->fileOffset);
        return;
    }

    // else try compensate and add
    bool compensationSucceeded = tryCompensateAndAdd(node->parent, middleKey, value, newNode->fileOffset);
    if (compensationSucceeded) return;

    // else split parent
    splitAndAddRecord(node->parent, middleKey, value, newNode->fileOffset);

}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::ostream &
operator<<(std::ostream &os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree) {
    return os << "Printing whole B-Tree:\n" << *bPlusTree.root << '\n'; //TODO: implement printing whole tree
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::updateConfigHeader() {
    this->fileHandle.seekp(0);
    configHeader.rootOffset = this->root->fileOffset;
    this->fileHandle.write((char *) &configHeader, sizeof(configHeader));
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
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::resetDiskOperationsCounters() {
    sessionDiskReadsCount = sessionDiskWritesCount = currentOperationDiskReadsCount = currentOperationDiskWritesCount = 0;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
template<typename TResult>
TResult BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::diskRead(size_t offset) {
    auto size = sizeof(TResult);
    auto result = this->diskRead(offset, size);
    return *reinterpret_cast<TResult *>(result.data()); // TODO: possibly wrong????
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::vector<char> BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::diskRead(size_t offset, size_t size) {
    if (this->fileHandle.bad()) {
        throw std::runtime_error(
                "Disk read at offset" + std::to_string(offset) + " of size " + std::to_string(size) + " failed before");
    }
    this->incrementReadOperationsCounters();
    this->fileHandle.clear();
    std::vector<char> result;
    result.resize(size);
    this->fileHandle.seekg(offset);
    this->fileHandle.read(result.data(), size);
    if (this->fileHandle.bad())
        throw std::runtime_error(
                "Disk read at offset" + std::to_string(offset) + " of size " + std::to_string(size) + " failed after");
    return result;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
template<typename TData>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::diskWrite(size_t offset, TData const &data) {
    auto dataVector = std::vector<char>();
    dataVector.resize(sizeof(TData));
    *reinterpret_cast<TData *>(dataVector.data()) = data;
    this->diskWrite(offset, dataVector);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::diskWrite(size_t offset, std::vector<char> const &data) {
    if (this->fileHandle.bad()) {
        throw std::runtime_error(
                "Disk write at offset" + std::to_string(offset) + " of size " + std::to_string(data.size()) +
                " failed before");
    }
    this->incrementWriteOperationsCounters();
    this->fileHandle.clear();
    this->fileHandle.seekp(offset);
    this->fileHandle.write(data.data(), data.size());
    if (this->fileHandle.bad())
        throw std::runtime_error(
                "Disk write at offset" + std::to_string(offset) + " of size " + std::to_string(data.size()) +
                " failed after");
}


#endif //SBD2_B_PLUS_TREE_HH
