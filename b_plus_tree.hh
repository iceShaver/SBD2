
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
#include "node.hh"
#include "inner_node.hh"
#include "leaf_node.hh"
#include "record.hh"
#include <graphviz/gvc.h>

namespace fs = std::filesystem;


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

    BPlusTree(fs::path filePath, OpenMode openMode = OpenMode::USE_EXISTING);

    static std::shared_ptr<ANode> ReadNode(std::fstream &fileHandle, size_t fileOffset);
    size_t AllocateDiskMemory(NodeType nodeType);
    BPlusTree &addRecord(TKey const &key, TValue const &value, std::shared_ptr<ANode> node = nullptr);
    void addRecordV2(TKey const &key, TValue const &value);
    size_t findProperDescendantOffset(std::shared_ptr<ANode> node, TKey const &key);
    bool tryCompensateAndAdd(std::shared_ptr<ANode> node,
                             TKey const &key, TValue const &value, size_t nodeOffset = 0);
    void splitAndAddRecord(std::shared_ptr<ANode> node,
                           TKey const &key, TValue const &value, size_t addedNodeOffset = 0);
    std::shared_ptr<ANode> getNodeUnfilledNeighbour(std::shared_ptr<ANode> node);
    BPlusTree &printTree();
    std::stringstream gvcPrintTree();
    void printNodeAndDescendants(std::shared_ptr<ANode> node);
    std::stringstream& gvcPrintNodeAndDescendants(std::shared_ptr<ANode> node, std::stringstream &ss);
    auto getAllocationsCounter() const { return this->allocationsCounter; }
    friend std::ostream &operator<<<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>(
            std::ostream &os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree);
    void display();
private:
    fs::path filePath;
    std::fstream fileHandle;
    std::shared_ptr<ANode> root;
    ConfigHeader configHeader;
    BPlusTree &updateConfigHeader();
    uint64_t allocationsCounter = 0;
};


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::BPlusTree(fs::path filePath, OpenMode openMode)
        : filePath(std::move(filePath)), configHeader() {
    debug([] { std::clog << "L: " << ALeafNode::BytesSize() << " I: " << AInnerNode::BytesSize() << '\n'; });
    this->fileHandle.rdbuf()->pubsetbuf(0, 0);
    switch (openMode) {
        case OpenMode::USE_EXISTING:
            if (!fs::is_regular_file(this->filePath))
                throw std::runtime_error("Couldn't open file: " + fs::absolute(this->filePath).string() + '\n');
            debug([this] { std::clog << "Opening file: " << fs::absolute(this->filePath) << '\n'; });
            this->fileHandle.open(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::ate);
            if (this->fileHandle.bad())
                throw std::runtime_error("Couldn't open file: " + fs::absolute(this->filePath).string() + '\n');
            this->fileHandle.seekg(0);
            this->fileHandle.seekp(0);
            this->fileHandle.read((char *) &configHeader, sizeof(configHeader));
            this->root = BPlusTree::ReadNode(fileHandle, configHeader.rootOffset);
            break;

        case OpenMode::CREATE_NEW:
            this->fileHandle.open(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
            if (!this->fileHandle.good())
                throw std::runtime_error("Error opening file: " + fs::absolute(this->filePath).string());
            debug([this] { std::clog << "Creating new db using file: " << fs::absolute(this->filePath) << '\n'; });
            this->root = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
            this->updateConfigHeader();
            break;
    }

    debug([this] { std::clog << "Root: " << *this->root << '\n'; }, 3);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::~BPlusTree() {
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
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ReadNode(std::fstream &fileHandle, size_t fileOffset) {
    // TODO: changeit to read only one time
    fileHandle.seekg(fileOffset);
    char header;
    fileHandle.read(&header, 1);
    if (std::bitset<8>(header)[0] == true) throw std::runtime_error("Tried to read empty node!");
    std::shared_ptr<ANode> result = nullptr;
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::INNER))
        result = std::make_shared<AInnerNode>(fileOffset, fileHandle);
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
        result = std::make_shared<ALeafNode>(fileOffset, fileHandle);
    result->load();
    return result;
}


// TODO: use some index to make it work faster
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
size_t
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::AllocateDiskMemory(NodeType nodeType) {
    // if (specifiedOffset) return *specifiedOffset + sizeof(configHeader);
    std::fpos<mbstate_t> result;
    this->fileHandle.seekg(sizeof(configHeader));
    char header;
    while (true) {
        this->fileHandle.read(&header, 1);

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
        if (std::bitset<8>(header)[1] == static_cast<int>(nodeType) && std::bitset<8>(header)[0] == true) {
            result = this->fileHandle.tellg() - 1;
            break;
        }

        // not empty -> search next
        auto stepSize = (std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
                        ? ALeafNode::BytesSize()
                        : AInnerNode::BytesSize();
        int64_t currentPosition = this->fileHandle.tellg() - 1;
        this->fileHandle.seekg(currentPosition + stepSize);
    }
    if (result < 0) throw std::runtime_error("Unable to allocate disk memory");
    auto offset = static_cast<size_t>(result);

    // This is only for purpose of mark space as occupied, TODO: do something more efficient e.g.: create header only
    this->fileHandle.seekp(offset);
    std::bitset<8> newHeader;
    newHeader[1] = static_cast<int>(nodeType);
    newHeader[0] = false; // not empty;
    char headerByte = static_cast<char>(newHeader.to_ulong());
    this->fileHandle.write(&headerByte, 1);
    // it is for extending the file to needed size, TODO: do something better
    if (nodeType == NodeType::LEAF) ALeafNode(offset, this->fileHandle).markChanged();
    else AInnerNode(offset, this->fileHandle).markChanged();
    this->fileHandle.flush();
    this->allocationsCounter++;
    return offset;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::addRecord(TKey const &key, TValue const &value,
                                                                      std::shared_ptr<ANode> node) {
    this->addRecordV2(key, value);
    /*if (!node) node = this->root;
    //else node->load();// TODO: check if this is necessary

    debug([&] {
        std::clog << "Adding record to node at: " << node->fileOffset << '\n';
        std::clog << *node << '\n'; // Continue here
    }, 2);

    if (node->nodeType() == NodeType::LEAF) {
        auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);
        if (leafNode->full()) {
            if (!tryCompensateAndAdd(node, key, value)) {
                splitAndAddRecord(node, key, value);
            }
        } else
            leafNode->insert(key, value);

    } else if (node->nodeType() == NodeType::INNER) { // if inner node -> go deeper
        // find proper descendant, load it, fill with parent pointer
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        // find key >= inserted key
        auto foundLowerBound = std::lower_bound(innerNode->keys.begin(), innerNode->keys.end(), key,
                                                [](auto element, auto value) {
                                                    if (!element) return false;
                                                    return *element < value;
                                                });

        auto ptrIndex = foundLowerBound - innerNode->keys.begin();

        auto descendantOffset = innerNode->descendants[ptrIndex];
        std::shared_ptr<ANode> descendant = nullptr;
        if (descendantOffset == std::nullopt) // TODO: think of it
            throw std::runtime_error("Internal db error: descendant node not found");

        descendant = BPlusTree::ReadNode(this->fileHandle, *descendantOffset);
        descendant->parent = node;
        this->addRecord(key, value, descendant);
    }*/
    return *this;
}


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
        auto result = BPlusTree::ReadNode(this->fileHandle, *leftNodeOffset);
        if (!result->full()) return result;
    }

    auto rightNodeOffset = (currentNodeOffsetIndex < parent->descendants.size() - 1)
                           ? parent->descendants[currentNodeOffsetIndex + 1]
                           : std::nullopt;

    // right neighbour found
    if (rightNodeOffset) {
        auto result = BPlusTree::ReadNode(this->fileHandle, *rightNodeOffset);
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
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printTree() {
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
            printNodeAndDescendants(BPlusTree::ReadNode(this->fileHandle, descOffset));
            std::cout << ' ';
            std::cout.flush();
        }
    }
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::addRecordV2(TKey const &key, TValue const &value) {

    // find leaf to insert record into
    std::shared_ptr<ANode> node = root;
    while (node->nodeType() != NodeType::LEAF) {
        auto descendantOffset = findProperDescendantOffset(node, key);
        auto nextNode = ReadNode(this->fileHandle, descendantOffset);
        nextNode->parent = node;
        node = nextNode;
    }
    auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);

    // if key exists then exit
    if (leafNode->keyExists(key)) {
        std::cout << "Given key already exists. Record not added.\n";
        return;
    }

    // if node not full -> insert record
    if (!node->full()) {
        leafNode->insert(key, value);
        return;
    }

    // else try compensate node and add record
    bool compensationSucceeded = tryCompensateAndAdd(node, key, value);
    if (compensationSucceeded) return;

    // else split node and add record
    splitAndAddRecord(node, key, value);
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
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::display() {
    GVC_t *gvc;
    Agraph_t *g;
    gvc = gvContext();

    auto str = R"**(
digraph g {
node [shape = record,height=.1];
node0[label = "<f0> |10|<f1> |20|<f2> |30|<f3>"];
node1[label = "<f0> |1|<f1> |2|<f2>"];
"node0":f0 -> "node1"
node2[label = "<f0> |11|<f1> |12|<f2>"];
"node0":f1 -> "node2"
node3[label = "<f0> |21|<f1> |22|<f2>"];
"node0":f2 -> "node3"
node4[label = "<f0> |31|<f1> |32|<f2>"];
"node0":f3 -> "node4"

}
    )**";
    g = agmemread(this->gvcPrintTree().str().c_str());
    std::string filePath = tmpnam(nullptr);
    auto file = fopen(filePath.c_str(), "w");
    gvLayout(gvc, g, "dot");

    gvRender(gvc, g, "png", file);
    gvFreeLayout(gvc, g);
    agclose(g);
    system(("xdg-open " + filePath).c_str());
    fclose(file);
    gvFreeContext(gvc);
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::stringstream
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::gvcPrintTree() {
    std::stringstream ss;
    ss << R"**(
digraph g{
node [shape = record,height=.1];
)**";

    gvcPrintNodeAndDescendants(this->root, ss);
    ss << "}";
    std::cout << ss.str() << std::endl;
    return ss;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::stringstream&
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::gvcPrintNodeAndDescendants(
        std::shared_ptr<BPlusTree::ANode> node, std::stringstream &ss) {
    ss << *node;
    if (node->nodeType() == NodeType::INNER) {
        auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
        for (auto &descOffset : innerNode->getEntries().second) {
            gvcPrintNodeAndDescendants(BPlusTree::ReadNode(this->fileHandle, descOffset), ss);
        }
    }
    return ss;
}


#endif //SBD2_B_PLUS_TREE_HH
