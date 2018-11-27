#include <iostream>
#include "b_plus_tree.hh"
#include "record.hh"
#include <memory>

int main() {
    return 0;
}

void test(){
    BPlusTree<int, Record, 10, 10> t("test.bin", OpenMode::CREATE_NEW);
}