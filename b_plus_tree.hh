#include <utility>

//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_B_PLUS_TREE_HH
#define SBD2_B_PLUS_TREE_HH

#include <string_view>
#include <filesystem>
#include <fstream>
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


    static std::unique_ptr<ANode> ReadNode(std::fstream &fileHandle, size_t fileOffset);
    size_t AllocateDiskMemory(NodeType nodeType, std::optional<size_t> specifiedOffset = std::nullopt);

    void test();


    BPlusTree &addRecord(TKey const &key, TValue const &value, std::shared_ptr<ANode> node = nullptr);
    bool tryCompensate(std::shared_ptr<ALeafNode> node, TKey const &key, TValue const &value);
    bool tryCompensate(std::shared_ptr<AInnerNode> node, TKey const &key);
    BPlusTree &split(std::shared_ptr<ALeafNode> node, TKey const &key, TValue const &value);
    BPlusTree &split(std::shared_ptr<AInnerNode> node, TKey const &key, size_t descendantOffset);
    std::pair<std::shared_ptr<ANode>, std::shared_ptr<ANode>>
    getNodeNonFullNeighbours(std::shared_ptr<ANode> node);
    BPlusTree &printAll();

    friend std::ostream &operator<<<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>(std::ostream&os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>const&bPlusTree);

private:
    fs::path filePath;
    std::fstream fileHandle;
    std::shared_ptr<ANode> root;
};

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::BPlusTree(fs::path filePath, OpenMode openMode)
        : filePath(std::move(filePath)) {
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
            this->root = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF, 0), this->fileHandle);
            this->root->load();
            break;

        case OpenMode::CREATE_NEW:
            this->fileHandle.open(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
            if (!this->fileHandle.good())
                throw std::runtime_error("Error opening file: " + fs::absolute(this->filePath).string());
            debug([this] { std::clog << "Creating new db using file: " << fs::absolute(this->filePath) << '\n'; });
            this->root = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF, 0), this->fileHandle);
            break;
    }

    debug([this] { std::clog << *this->root << '\n'; });
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
void BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::test() {
    std::clog << "New node values\n";
    ((ALeafNode *) root.get())->values[0] = Record{21, 45, 32};
    ((ALeafNode *) root.get())->keys[0] = 6969;

    this->addRecord(123, Record{12, 43, 54});
    //std::cout << *root << std::endl;



}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::unique_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ReadNode(std::fstream &fileHandle, size_t fileOffset) {
    fileHandle.seekg(fileOffset);
    char header;
    fileHandle.read(&header, 1);
    if (std::bitset<8>(header)[0] == true) throw std::runtime_error("Tried to read empty node!");
    std::unique_ptr<ANode> result = nullptr;
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::INNER))
        result = std::make_unique<AInnerNode>(InnerNode<TKey, TValue, TInnerNodeDegree>(fileOffset, fileHandle));
    if (std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
        result = std::make_unique<ALeafNode>(LeafNode<TKey, TValue, TLeafNodeDegree>(fileOffset, fileHandle));
    result->load();
    return result;
}

// TODO: use some index to make it work faster
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
size_t
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::AllocateDiskMemory(NodeType nodeType,
                                                                               std::optional<size_t> specifiedOffset) {
    if (specifiedOffset) return *specifiedOffset;
    std::fpos<mbstate_t> result;
    this->fileHandle.seekg(0);
    char header;
    while (true) {
        this->fileHandle.read(&header, 1);
        if (this->fileHandle.eof()) {
            this->fileHandle.clear();
            result = this->fileHandle.tellg();
            break;
        }
        if (std::bitset<8>(header)[1] == static_cast<int>(nodeType) && std::bitset<8>(header)[0] == true) {
            result = this->fileHandle.tellg() - 1;
            break;
        }
        this->fileHandle.seekg((size_t) this->fileHandle.tellg() +
                               ((std::bitset<8>(header)[1] == static_cast<int>(NodeType::LEAF))
                                ? ALeafNode::BytesSize() : AInnerNode::BytesSize()) - 1);
    }
    if (result < 0) throw std::runtime_error("Unable to allocate disk memory");
    auto offset = static_cast<size_t>(result);
    // This is only for purpose of mark space as occupied, TODO: do something more efficient e.g.: create header only
    if (nodeType == NodeType::LEAF) ALeafNode(offset, this->fileHandle).unload();
    else AInnerNode(offset, this->fileHandle).unload();
    return offset;
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::addRecord(TKey const &key, TValue const &value,
                                                                      std::shared_ptr<ANode> node) {
    if (!node) node = this->root;
    else node->load();

    if (node->nodeType() == NodeType::LEAF) {
        auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);
        if (leafNode->full()) {
            if (!this->tryCompensate(leafNode, key, value)) {  // if not -> split
                this->split(leafNode, key, value);
            }
        } else {// leaf node is not full
            leafNode->insert(key, value);
        }
    } else if (node->nodeType() == NodeType::INNER) {
        // find proper descendant, read it, fill with parent pointer
        // addRecord(key, value, descendant)
    }

    return *this;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
bool BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::tryCompensate(std::shared_ptr<ALeafNode> node,
                                                                               TKey const &key, TValue const &value) {
    if (node->parent == nullptr) return false;
    auto[leftNeighbour, rightNeighbour] = this->getNodeNonFullNeighbours(node);
    std::shared_ptr<ANode> selectedNeighbour = nullptr;
    if (leftNeighbour) selectedNeighbour = leftNeighbour;
    else if (rightNeighbour) selectedNeighbour = rightNeighbour;

    if (selectedNeighbour == nullptr) return false;

    // compensate leaf node
    auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);
    // compensate with left node
    if (leftNeighbour && !leftNeighbour->full()) {
        auto leftNode = std::dynamic_pointer_cast<ALeafNode>(leftNeighbour);
        std::vector<std::pair<TKey, TValue>> allData = leftNode->getRecords();
        std::vector<std::pair<TKey, TValue>> data = leafNode->getRecords();
        allData.insert(allData.end(), data.begin(), data.end());
        allData.insert(std::upper_bound(allData.begin(), allData.end(), std::pair(key, value),
                                        [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
        auto mid = allData.size() / 2;
        auto parentNode = std::dynamic_pointer_cast<AInnerNode>(node->parent);
        parentNode->changeKey(leftNode->fileOffset, leafNode->fileOffset, allData[mid].first);
        leftNode->setRecords(allData.begin(), allData.begin() + mid + 1);
        leafNode->setRecords(allData.begin() + mid + 1, allData.end());
        // compensate with right node
    } else if (rightNeighbour && !rightNeighbour->full()) {
        auto rightNode = std::dynamic_pointer_cast<ALeafNode>(rightNeighbour);
        std::vector<std::pair<TKey, TValue>> allData = leafNode->getRecords();
        std::vector<std::pair<TKey, TValue>> data = rightNode->getRecords();
        allData.insert(allData.end(), data.begin(), data.end());
        allData.insert(std::upper_bound(allData.begin(), allData.end(), std::pair(key, value),
                                        [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
        auto mid = allData.size() / 2;
        auto parentNode = std::dynamic_pointer_cast<AInnerNode>(node->parent);
        parentNode->changeKey(leafNode->fileOffset, rightNode->fileOffset, allData[mid].first);
        leafNode->setRecords(allData.begin(), allData.begin() + mid + 1);
        rightNode->setRecords(allData.begin() + mid + 1, allData.end());
    }

}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
bool BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::tryCompensate(std::shared_ptr<AInnerNode> node,
                                                                               TKey const &key) {
    if (node->parent == nullptr) return false;
    auto[leftNeighbour, rightNeighbour] = this->getNodeNonFullNeighbours(node);
    std::shared_ptr<ANode> selectedNeighbour = nullptr;
    if (leftNeighbour) selectedNeighbour = leftNeighbour;
    else if (rightNeighbour) selectedNeighbour = rightNeighbour;

    if (selectedNeighbour == nullptr) return false;

    auto innerNode = std::dynamic_pointer_cast<AInnerNode>(node);
    auto parentNode = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    // compensate with left node
    if (leftNeighbour && !leftNeighbour->full()) {
        auto leftNode = std::dynamic_pointer_cast<AInnerNode>(leftNeighbour);
        // add left node
        std::pair<std::vector<TKey>, std::vector<size_t>> allData = leftNode->getEntries();
        std::pair<std::vector<TKey>, std::vector<size_t>> data = innerNode->getEntries();
        // add key from parent
        allData.first.push_back(parentNode->getKeyBetweenPointers(leftNode->fileOffset, innerNode->fileOffset));
        // add right node
        allData.first.insert(allData.first.end(), data.first.begin(), data.first.end());
        allData.second.insert(allData.second.end(), data.second.begin(), data.second.end());
        // add added key
        allData.first.insert(std::upper_bound(allData.first.begin(), allData.first.end(), key), key);

        // select keysMid key
        auto keysMid = allData.first.size() / 2;
        auto descMid = allData.first.size() / 2;

        // put keysMid key to parent
        parentNode->changeKey(leftNode->fileOffset, innerNode->fileOffset, allData.first[keysMid]);

        // split elements to both nodes by keysMid key
        leftNode->setKeys(allData.first.begin(), allData.first.begin() + keysMid);
        innerNode->setKeys(allData.first.begin() + keysMid + 1, allData.first.end());
        leftNode->setDescendants(allData.second.begin(), allData.second.begin() + descMid);
        innerNode->setDescendants(allData.second.begin() + descMid, allData.second.end());

        // compensate with right node
    } else if (rightNeighbour && !rightNeighbour->full()) {
        auto rightNode = std::dynamic_pointer_cast<AInnerNode>(rightNeighbour);
        // add right node
        std::pair<std::vector<TKey>, std::vector<size_t>> allData = innerNode->getEntries();
        std::pair<std::vector<TKey>, std::vector<size_t>> data = rightNode->getEntries();
        // add key from parent
        allData.first.push_back(parentNode->getKeyBetweenPointers(rightNode->fileOffset, innerNode->fileOffset));
        // add left node
        allData.first.insert(allData.first.end(), data.first.begin(), data.first.end());
        allData.second.insert(allData.second.end(), data.second.begin(), data.second.end());
        // add added key
        allData.first.insert(std::upper_bound(allData.first.begin(), allData.first.end(), key), key);

        // select keysMid key
        auto keysMid = allData.first.size() / 2;
        auto descMid = allData.first.size() / 2;

        // put keysMid key to parent
        parentNode->changeKey(rightNode->fileOffset, innerNode->fileOffset, allData.first[keysMid]);

        // split elements to both nodes by keysMid key
        innerNode->setKeys(allData.first.begin(), allData.first.begin() + keysMid);
        rightNode->setKeys(allData.first.begin() + 1 + keysMid, allData.first.end());
        innerNode->setDescendants(allData.second.begin(), allData.second.begin() + descMid);
        rightNode->setDescendants(allData.second.begin() + descMid, allData.second.end());
    }

}
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
std::pair<std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>,
        std::shared_ptr<typename BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::ANode>>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::getNodeNonFullNeighbours(
        std::shared_ptr<ANode> node) {
    if (node->parent == nullptr) throw std::runtime_error("Cannot get node neighbours if it has no parent");
    std::shared_ptr<AInnerNode> parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);
    auto currentNodeIndex = std::find(parent->keys.begin(), parent->keys.end(), node->fileOffset);
    std::optional<size_t> leftNodeOffset = std::nullopt;
    std::optional<size_t> rightNodeOffset = std::nullopt;
    if (currentNodeIndex != parent->keys.begin()) leftNodeOffset = *(currentNodeIndex - 1);
    if (currentNodeIndex < parent->keys.end() - 1) rightNodeOffset = *(currentNodeIndex + 1);

    std::shared_ptr<ANode> leftNodePtr = nullptr;
    std::shared_ptr<ANode> rightNodePtr = nullptr;
    if (node->nodeType() == NodeType::LEAF) {
        if (leftNodeOffset)
            leftNodePtr = std::make_shared<ALeafNode>(*leftNodeOffset, this->fileHandle), leftNodePtr->load();
        if (rightNodeOffset)
            rightNodePtr = std::make_shared<ALeafNode>(*rightNodeOffset, this->fileHandle), leftNodePtr->load();
    } else if (node->nodeType() == NodeType::INNER) {
        if (leftNodeOffset)
            leftNodePtr = std::make_shared<AInnerNode>(*leftNodeOffset, this->fileHandle), leftNodePtr->load();
        if (rightNodeOffset)
            rightNodePtr = std::make_shared<AInnerNode>(*rightNodeOffset, this->fileHandle), leftNodePtr->load();
    }
    if (leftNodePtr->full()) leftNodePtr = nullptr;
    if (rightNodePtr->full()) rightNodePtr = nullptr;
    return std::pair(std::move(leftNodePtr), std::move(rightNodePtr));
}

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::split(std::shared_ptr<ALeafNode> node,
                                                                  TKey const &key, TValue const &value) {
    // TODO: handle root
    auto newNode = std::make_shared<ALeafNode>(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
    auto oldNode = std::dynamic_pointer_cast<ALeafNode>(node);
    std::vector<std::pair<TKey, TValue>> data = oldNode->getRecords();
    data.insert(std::upper_bound(data.begin(), data.end(), std::pair(key, value),
                                 [](auto x, auto y) { return x.first < y.first; }), std::pair(key, value));
    auto mid = data.size() / 2;
    auto midKey = data[mid].first;
    oldNode->setRecords(data.begin(), data.begin() + mid + 1);
    newNode->setRecords(data.begin() + 1, data.end());
    auto parent = std::dynamic_pointer_cast<AInnerNode>(node->parent);

    if (parent->full()) {
        if (!this->tryCompensate(parent, midKey)) {
            this->split(parent, midKey, newNode->fileOffset);
        }
    } else {
        parent->add(midKey, newNode->fileOffset);
    }
    return *this;
}


template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::printAll() {
    debug([] { std::clog << "printAll\n"; });
    return *this;
}
template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> &
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::split(std::shared_ptr<AInnerNode> node,
                                                                  TKey const &key, size_t descendantOffset) {
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
        if (!this->tryCompensate(parent, midKey)) {
            this->split(parent, midKey, newNode->fileOffset);
        }
    } else {
        parent->add(midDesc, newNode->fileOffset);
    }
    return *this;
}
template <typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>std::ostream&operator<<(std::ostream&os, BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree> const &node){
    return os << "Printing whole B-Tree:\n";
}

#endif //SBD2_B_PLUS_TREE_HH
