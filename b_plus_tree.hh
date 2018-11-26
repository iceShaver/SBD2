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


    void test();

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
            this->root = std::make_shared<ALeafNode>(0, this->fileHandle);
            this->root->load();
            break;

        case OpenMode::CREATE_NEW:
            this->fileHandle.open(this->filePath, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
            if (!this->fileHandle.good())
                throw std::runtime_error("Error opening file: " + fs::absolute(this->filePath).string());
            debug([this] { std::clog << "Creating new db using file: " << fs::absolute(this->filePath) << '\n'; });
            this->root = std::make_shared<ALeafNode>(0, this->fileHandle);

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
    if (std::bitset<8>(header)[1] == 0)
        result = std::make_unique<AInnerNode>(InnerNode<TKey, TValue, TInnerNodeDegree>(fileOffset, fileHandle));
    if (std::bitset<8>(header)[1] == 1)
        result = std::make_unique<ALeafNode>(LeafNode<TKey, TValue, TLeafNodeDegree>(fileOffset, fileHandle));
    result->load();
    return result;
}


#endif //SBD2_B_PLUS_TREE_HH
