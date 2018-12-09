#include <iostream>
#include "b_plus_tree.hh"
#include "record.hh"
#include <memory>
#include <filesystem>

#include <graphviz/gvc.h>
namespace fs = std::filesystem;

//TODO: remove all flushes
void test();

int main() {
    test();

    return 0;
}

void test() {
    BPlusTree<int, Record, 1, 1> t("test.bin", OpenMode::CREATE_NEW);
    std::cout << "Add: " << 20<<'\n';t.addRecord(20, Record{25, 58, 69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 50<<'\n';t.addRecord(50, Record{12, 58, 45});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 40<<'\n';t.addRecord(40, Record{45, 78, 78});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 80<<'\n';t.addRecord(80, Record{25, 65, 12});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 70<<'\n';t.addRecord(70, Record{32,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 30<<'\n';t.addRecord(30, Record{25,65,98});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 10<<'\n';t.addRecord(10, Record{21,58,69});t.printTree();std::cout << '\n';
    t.printTree();
    std::cout << "Add: " << 0<<'\n';t.addRecord(0, Record::random());t.printTree();std::cout << '\n';
    std::cout << "Add: " << 15<<'\n';t.addRecord(15, Record::random());t.printTree();std::cout << '\n';
    std::cout << "Add: " << 39<<'\n';t.addRecord(39, Record::random());t.printTree();std::cout << '\n';
    std::cout << "Add: " << 1000<<'\n';t.addRecord(1000, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << -1000<<'\n';t.addRecord(-1000, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << -555<<'\n';t.addRecord(-555, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 9999<<'\n';t.addRecord(9999, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 453<<'\n';t.addRecord(453, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 543<<'\n';t.addRecord(543, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 123<<'\n';t.addRecord(123, Record{25,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 12<<'\n';t.addRecord(12, Record{12,58,45});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 456<<'\n';t.addRecord(456, Record{45,78,78});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 789<<'\n';t.addRecord(789, Record{25,65,12});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 987<<'\n';t.addRecord(987, Record{32,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 3147<<'\n';t.addRecord(3147, Record{25,65,98});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 1654<<'\n';t.addRecord(1654, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 1000546<<'\n';t.addRecord(1000546, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << -100045<<'\n';t.addRecord(-100045, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 4531<<'\n';t.addRecord(4531, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 5431<<'\n';t.addRecord(5431, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << t.getAllocationsCounter() << '\n';
    t.display();
}