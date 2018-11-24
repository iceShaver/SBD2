//
// Created by kamil on 24.11.18.
//

#ifndef SBD2_B_PLUS_TREE_HH
#define SBD2_B_PLUS_TREE_HH

#include <string_view>
#include <filesystem>
#include <fstream>
#include "node.hh"
#include "inner_node.hh"
#include "leaf_node.hh"


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

    BPlusTree(fs::path const &file_path, OpenMode openMode = OpenMode::USE_EXISTING);


private:
    ANode root;
    fs::path file_path;
    std::fstream fileHandle;
};

template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree>
BPlusTree<TKey, TValue, TInnerNodeDegree, TLeafNodeDegree>::BPlusTree(fs::path const &file_path, OpenMode openMode)
: root(), file_path(file_path), fileHandle{
    switch (openMode) {
        case OpenMode::USE_EXISTING:
            break;
        case OpenMode::CREATE_NEW:
            if(fs::is_regular_file(file_path))
            break;
    }

}


#endif //SBD2_B_PLUS_TREE_HH
