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
    /*if (root->nodeType() == NodeType::LEAF) {
        auto node = (ALeafNode *) root.get();
        node->values[0] = Record{21, 45, 32};
        node->keys[0] = 6969;
    } else if (root->nodeType() == NodeType::INNER) {

    }*/
    //std::cout << *root << '\n';
    std::clog << "New node values\n";
    ((ALeafNode *) root.get())->values[0] = Record{21, 45, 32};
    ((ALeafNode *) root.get())->keys[0] = 6969;
    //root->unload();
    //root->load();
    //ALeafNode leaf(0, fileHandle);
    //auto leaf = ReadNode(fileHandle, 0);
    //leaf.load();
    //std::cout << *leaf << '\n';
    auto n1 = ALeafNode(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
    auto n2 = ALeafNode(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
    auto n3 = ALeafNode(AllocateDiskMemory(NodeType::LEAF), this->fileHandle);
    auto n4 = AInnerNode(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
    auto n5 = AInnerNode(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
    auto n6 = AInnerNode(AllocateDiskMemory(NodeType::INNER), this->fileHandle);
    n2.markEmpty().unload();
    n5.markEmpty().unload();
    n6.markEmpty().unload();
    auto newOffset = AllocateDiskMemory(NodeType::LEAF);
    auto ne1wOffset = AllocateDiskMemory(NodeType::LEAF);
    auto new2Offset = AllocateDiskMemory(NodeType::LEAF);
    auto newO3ffset = AllocateDiskMemory(NodeType::INNER);
    auto newOf4fset = AllocateDiskMemory(NodeType::INNER);
    auto newOff5set = AllocateDiskMemory(NodeType::INNER);
    //leaf->printData(std::cout);


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
    if (node->nodeType() == NodeType::LEAF) {
        auto leafNode = std::dynamic_pointer_cast<ALeafNode>(node);
        if (leafNode->isFull()) {
            // try to compensate
            // if not -> split
        } else { // leaf node is not full
            leafNode.insert(key, value);
        }
    } else if(node->nodeType() == NodeType::INNER){
        // find proper descendant, read it, fill with parent pointer
        // addRecord(key, value, descendant)
    }

    return *this;
}


#endif //SBD2_B_PLUS_TREE_HH
