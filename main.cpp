#include <iostream>
#include "b_plus_tree.hh"
#include "record.hh"
#include <memory>

void test();

int main() {
    test();
    return 0;
}

void test(){
    BPlusTree<int, Record, 3, 3> t("test.bin", OpenMode::CREATE_NEW);
    t.addRecord(2, Record{25,58,69});
    t.addRecord(5, Record{12,58,45});
    t.addRecord(4, Record{45,78,78});
    t.addRecord(8, Record{25,65,12});
    t.addRecord(7, Record{32,58,69});
    t.addRecord(3, Record{25,65,98});
    t.addRecord(1, Record{21,58,69});
    t.addRecord(1000, Record{21,58,69});
    t.addRecord(-1000, Record{21,58,69});
    t.addRecord(453, Record{21,58,69});
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
    t.addRecord(5431, Record{21,58,69});
    std::cout << t;
}