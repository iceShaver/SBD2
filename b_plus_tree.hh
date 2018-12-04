#include <utility>

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
#include "node.hh"
#include "inner_node.hh"
#include "leaf_node.hh"
#include "record.hh"


namespace fs = std::filesystem;

enum class OpenMode { USE_EXISTING, CREATE_NEW };

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree> class BPlusTree;

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::ostream &operator<<(std::ostream &, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &);

struct ConfigHeader {
    uint64_t rootOffset;
};

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree> class BPlusTree final {
    using ANode = Node<TKey, TValue>;
    using AInnerNode = InnerNode<TKey, TValue, TInnerNodeDegree>;
    using ALeafNode = LeafNode<TKey, TValue, TLeafNodeDegree>;


public:
    BPlusTree() = delete;
    BPlusTree(BPlusTree &&) = delete;
    BPlusTree(BPlusTree const &) = delete;
    BPlusTree &operator=(BPlusTree &&) = delete;
    BPlusTree &operator=(BPlusTree const &) = delete;

    BPlusTree(fs::path filePath, OpenMode openMode = OpenMode::USE_EXISTING);

    ~BPlusTree();

    static std::shared_ptr<ANode> ReadNode(std::fstream &fileHandle, size_t fileOffset);
    size_t AllocateDiskMemory(NodeType nodeType);

    void test();


    BPlusTree &addRecord(TKey const &key, TValue const &value, std::shared_ptr<ANode> node = nullptr);
    bool
    tryCompensateAndAddRecord(std::shared_ptr<ANode> node, TKey const &key, TValue const &value, size_t nodeOffset = 0);
    //bool tryCompensateAndAddRecord(std::shared_ptr<AInnerNode> node, TKey const &key);
    BPlusTree &splitAndAddRecord(std::shared_ptr<ANode> node, TKey const &key, TValue const &value);
    BPlusTree &splitAndAddRecord(std::shared_ptr<ALeafNode> node, TKey const &key, TValue const &value);
    BPlusTree &splitAndAddRecord(std::shared_ptr<AInnerNode> node, TKey const &key, size_t descendantOffset);
    std::pair<std::shared_ptr<ANode>, std::shared_ptr<ANode>>
    getNodeNonFullNeighbours(std::shared_ptr<ANode> node);
    BPlusTree &printTree();
    void printNodeAndDescendants(std::shared_ptr<ANode> node);
    auto getAllocationsCounter() const { return this->allocationsCounter; }
    friend std::ostream &operator<<<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>(
            std::ostream &os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &bPlusTree);

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
        : filePath(std::move(filePath)) {
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
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::test() {
    std::clog << "New node values\n";
    ((ALeafNode *) root.get())->values[0] = Record{21, 45, 32};
    ((ALeafNode *) root.get())->keys[0] = 6969;

    this->addRecord(123, Record{12, 43, 54});
    //std::cout << *root << std::endl;



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
    if (!node) node = this->root;
    else node->load();// TODO: check if this is necessary
    debug([&] {
        std::clog << "Adding record to node at: " << node->fileOffset << '\n';
        std::clog << *node << '\n'; // Continue here
    }, 2);

    if (node->nodeType() == NodeType::LEAF) {
        auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);
        if (leafNode->full())
            tryCompensateAndAddRecord(node, key, value) || (splitAndAddRecord(node, key, value), 0);
        else
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
        if (foundLowerBound == innerNode->keys.end())
            throw std::runtime_error("Internal db error: descendant node not found");
        auto ptrIndex = foundLowerBound - innerNode->keys.begin();

        auto descendantOffset = innerNode->descendants[ptrIndex];
        std::shared_ptr<ANode> descendant = nullptr;
        if (descendantOffset == std::nullopt) // TODO: think of it
            throw std::runtime_error("Internal db error: descendant node not found");

        descendant = BPlusTree::ReadNode(this->fileHandle, *descendantOffset);
        descendant->parent = node;
        this->addRecord(key, value, descendant);
    }

    return *this;
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
bool
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::tryCompensateAndAddRecord(std::shared_ptr<ANode> node,
                                                                                      TKey const &key,
                                                                                      TValue const &value,
                                                                                      size_t nodeOffset) {
    if (!node) throw std::invalid_argument("Given node argument is nullptr");

    if (node->parent == nullptr) return false;
    auto[leftNeighbour, rightNeighbour] = this->getNodeNonFullNeighbours(node);
    std::shared_ptr<ANode> selectedNeighbour = nullptr;
    if (leftNeighbour) selectedNeighbour = leftNeighbour;
    else if (rightNeighbour) selectedNeighbour = rightNeighbour;

    if (selectedNeighbour == nullptr) return false;
    auto parentNode = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    // compensate with left node
    if (leftNeighbour && !leftNeighbour->full()) {
        auto midKey = leftNeighbour->compensateWithAndReturnMiddleKey(node, key, value, nodeOffset);
        parentNode->changeKey(leftNeighbour->fileOffset, midKey);
        // compensate with right node
    } else if (rightNeighbour && !rightNeighbour->full()) {
        auto midKey = node->compensateWithAndReturnMiddleKey(rightNeighbour, key, value, nodeOffset);
        parentNode->changeKey(node->fileOffset, midKey);
    }
    return true;
}


/**
 * Returns the tuple of pointers to left and right neighbour if they exist (nullptrs respectively if don't)
 * @tparam TKey
 * @tparam TValue
 * @tparam TInnerNodeDegree
 * @tparam TLeafNodeDegree
 * @param node Node which neighbours we want to find, node has to have the parent
 * @return tuple(leftNeighbour, rightNeighbour)
 */
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::pair<std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>,
        std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getNodeNonFullNeighbours(std::shared_ptr<ANode> node) {

    if (node->parent == nullptr)
        throw std::runtime_error("Cannot get node neighbours if it has no parent");

    std::shared_ptr<AInnerNode> parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
    auto currentNodeOffsetIterator = std::find(parent->descendants.begin(), parent->descendants.end(),
                                               node->fileOffset);

    if (currentNodeOffsetIterator == parent->descendants.end())
        throw std::runtime_error("Internal DB error: couldn't find ptr to current node at parent's");

    auto currentNodeOffsetIndex = currentNodeOffsetIterator - parent->descendants.begin();
    auto leftNodeOffset = (currentNodeOffsetIndex > 0)
                          ? parent->descendants[currentNodeOffsetIndex - 1]
                          : std::nullopt;
    auto rightNodeOffset = (currentNodeOffsetIndex < parent->descendants.size() - 1)
                           ? parent->descendants[currentNodeOffsetIndex + 1]
                           : std::nullopt;

    auto leftNodePtr = leftNodeOffset ? BPlusTree::ReadNode(this->fileHandle, *leftNodeOffset) : nullptr;
    auto rightNodePtr = rightNodeOffset ? BPlusTree::ReadNode(this->fileHandle, *rightNodeOffset) : nullptr;

    if (leftNodePtr && leftNodePtr->full()) leftNodePtr = nullptr;
    if (rightNodePtr && rightNodePtr->full()) rightNodePtr = nullptr;

    return std::pair(std::move(leftNodePtr), std::move(rightNodePtr));
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::splitAndAddRecord(std::shared_ptr<ALeafNode> node,
                                                                              TKey const &key, TValue const &value) {

    if (node == root) {
        auto newRoot = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
        auto newNode = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
        auto oldRoot = std::dynamic_pointer_cast<ALeafNode>(this->root);

        std::vector<std::pair<TKey, TValue>> data = oldRoot->getRecords();
        data.insert(std::upper_bound(data.begin(), data.end(), std::pair(key, value),
                                     [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
        auto mid = data.size() / 2;
        auto midKey = data[mid].first;
        newRoot->descendants[0] = oldRoot->fileOffset;
        newRoot->keys[0] = midKey;
        newRoot->descendants[1] = newNode->fileOffset;
        oldRoot->setRecords(data.begin(), data.begin() + mid + 1);
        newNode->setRecords(data.begin() + mid + 1, data.end());
        this->root = newRoot;
        return *this;
    }

    if (node->parent == nullptr) throw std::runtime_error("Internal database error: nullptr node parent");
    auto newNode = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
    auto oldNode = std::dynamic_pointer_cast<ALeafNode>(node);
    std::vector<std::pair<TKey, TValue>> data = oldNode->getRecords();
    data.insert(std::upper_bound(data.begin(), data.end(), std::pair(key, value),
                                 [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
    auto mid = data.size() / 2;
    auto midKey = data[mid].first;
    oldNode->setRecords(data.begin(), data.begin() + mid + 1);
    newNode->setRecords(data.begin() + mid + 1, data.end());
    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    if (parent->full()) {
        if (!this->tryCompensateAndAddRecord(node->parent, midKey, value)) {
            this->splitAndAddRecord(parent, midKey, newNode->fileOffset);
        }
    } else {
        parent->add(midKey, newNode->fileOffset);
    }
    return *this;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::splitAndAddRecord(std::shared_ptr<AInnerNode> node,
                                                                              TKey const &key,
                                                                              size_t descendantOffset) {
    // TODO: handle root
    auto newNode = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
    std::pair<std::vector<TKey>, std::vector<size_t>> data = node->getEntries();
    auto insertPosition = std::upper_bound(data.first.begin(), data.first.end(), key) - data.first.begin();
    data.first.insert(data.first.begin() + insertPosition, key);
    data.second.insert(data.second.begin() + insertPosition + 1, descendantOffset);
    auto keyMidPos = data.first.size() / 2;
    auto descMidPos = data.second.size() / 2;
    auto midKey = data.first[keyMidPos];
    auto midDesc = data.second[descMidPos];
    node->setKeys(data.first.begin(), data.first.begin() + keyMidPos);
    newNode->setKeys(data.first.begin() + keyMidPos + 1, data.first.end());
    node->setDescendants(data.second.begin(), data.second.begin() + descMidPos);
    newNode->setDescendants(data.second.begin() + descMidPos, data.second.end());
    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
    if (parent->full()) {
        if (!this->tryCompensateAndAddRecord(node->parent, midKey,
                                             Record::random())) { // TODO: change random to something more intelligent
            this->splitAndAddRecord(parent, midKey, newNode->fileOffset);
        }
    } else {
        parent->add(midDesc, newNode->fileOffset);
    }
    return *this;
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::splitAndAddRecord(std::shared_ptr<ANode> node,
                                                                              TKey const &key,
                                                                              TValue const &value) {
    if (node == root) {
        std::shared_ptr<ANode> newNode;
        if (node->nodeType() == NodeType::LEAF)
            newNode = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
        else
            newNode = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
        auto newRoot = std::make_shared<AInnerNode>(AllocateDiskMemory(NodeType::INNER), this->fileHandle);

        auto oldRoot = std::dynamic_pointer_cast<ALeafNode>(this->root);
        auto midKey = oldRoot->compensateWithAndReturnMiddleKey(newNode, key, value, 0);
        newRoot->descendants[0] = oldRoot->fileOffset;
        newRoot->keys[0] = midKey;
        newRoot->descendants[1] = newNode->fileOffset;
        this->root = newRoot;
        this->updateConfigHeader(); // TODO: check if this is necessary
        return *this;
    }
//    if (node->parent == nullptr) throw std::runtime_error("Internal database error: nullptr node parent");
//    auto middleKey = node->compensateWithAndReturnMiddleKey(newNeighbour, key, value, 0);
//    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
    //if(node->parent->full())
    //    tryCompensateAndAddRecord(node->parent, middleKey, value, newNeighbour->fileOffset);
    return *this;
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
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printNodeAndDescendants(std::shared_ptr<ANode> node) {
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
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::~BPlusTree() {
    this->updateConfigHeader();
}

#endif //SBD2_B_PLUS_TREE_HH
