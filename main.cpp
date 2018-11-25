#include <iostream>
#include "b_plus_tree.hh"
#include "record.hh"
#include <memory>

int main() {
    std::cout << "Hello, World!" << std::endl;

    BPlusTree<int, Record, 10, 10> t("test.bin", OpenMode::USE_EXISTING);
    t.test();
//    std::cout << sizeof(s) << std::endl;

    //auto ptr = std::shared_ptr<Node<int, int>>(InnerNode<int, int, 2>);
    //Node<int,int> n = InnerNode<int, int,5>();
    return 0;
}