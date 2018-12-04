#include <iostream>
#include "b_plus_tree.hh"
#include "record.hh"
#include <memory>

void test();

int main() {
    test();
    return 0;
}

void test() {
    BPlusTree<int, Record, 3, 3> t("test.bin", OpenMode::CREATE_NEW);
    t.addRecord(20, Record{25, 58, 69});t.printTree();std::cout << '\n';
    t.addRecord(50, Record{12, 58, 45});t.printTree();std::cout << '\n';
    t.addRecord(40, Record{45, 78, 78});t.printTree();std::cout << '\n';
    t.addRecord(80, Record{25, 65, 12});t.printTree();std::cout << '\n';
     t.addRecord(70, Record{32,58,69});t.printTree();std::cout << '\n';
     t.addRecord(30, Record{25,65,98});t.printTree();std::cout << '\n';
     t.addRecord(10, Record{21,58,69});t.printTree();std::cout << '\n';
     t.addRecord(0, Record::random());t.printTree();std::cout << '\n';
     t.addRecord(15, Record::random());t.printTree();std::cout << '\n';
     t.addRecord(39, Record::random());t.printTree();std::cout << '\n';
      //t.addRecord(1000, Record{21,58,69});t.printTree();std::cout << '\n';
      t.addRecord(-1000, Record{21,58,69});t.printTree();std::cout << '\n';
      t.addRecord(-555, Record{21,58,69});t.printTree();std::cout << '\n';
      t.addRecord(9999, Record{21,58,69});t.printTree();std::cout << '\n';
    /*  t.addRecord(453, Record{21,58,69});
      t.addRecord(543, Record{21,58,69});
      t.addRecord(123, Record{25,58,69});
      t.addRecord(12, Record{12,58,45});
      t.addRecord(456, Record{45,78,78});
      t.addRecord(789, Record{25,65,12});
      t.addRecord(987, Record{32,58,69});
      t.addRecord(3147, Record{25,65,98});
      t.addRecord(1654, Record{21,58,69});
      t.addRecord(1000546, Record{21,58,69});
      t.addRecord(-100045, Record{21,58,69});
      t.addRecord(4531, Record{21,58,69});
      t.addRecord(5431, Record{21,58,69});*/
    std::cout << "Printing tree:\n";
}