//
// Created by kamil on 10.12.18.
//

#include "dbms.hh"
#include "b_plus_tree.hh"
#include <readline/readline.h>
#include <readline/history.h>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <functional>

int Dbms::main(int argc, char **argv) {
    this->initCommands();

    this->commandLineLoop();


    this->test();
    return 0;
}


void Dbms::test() const {
    BPlusTree<int64_t, Record, 4, 3> t("test.bin", OpenMode::USE_EXISTING);
    /*for (int i = 0; i < 1000; ++i) {
        t.addRecord(UniqueGenerator<2000, 3000>::GetRandom(), Record::random());
    }
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
    std::cout << "Add: " << -555<<'\n';t.addRecord(-555, Record{21,58,69});t.print();std::cout << '\n';
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
    std::cout << "Add: " << 1046<<'\n';t.addRecord(1000546, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << -1045<<'\n';t.addRecord(-100045, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 4531<<'\n';t.addRecord(4531, Record{21,58,69});t.printTree();std::cout << '\n';
    std::cout << "Add: " << 5431<<'\n';t.addRecord(5431, Record{21,58,69});t.printTree();std::cout << '\n';*/
    t.draw();
}


void Dbms::initCommands() {
    // @formatter:off
    commands = {
            // dbms operations
            {"help",           {[&](auto params) { this->printHelp(); },            "Prints this help"}},
            {"exit",           {[&](auto params) { this->exit(); },                 "Close DB file and exit program"}},
            {"open_db_file",   {[&](auto params) { this->loadDbFile(params); },     "Open specified db file"}},
            {"create_db_file", {[&](auto params) { this->createDbFile(params); },   "Create new db file at specified location"}},
            {"close_db_file",  {[&](auto params) { this->closeDbFile(); },          "Save and close db file"}},
            {"print_db_file",  {[&](auto params) { this->printDbFile(); },          "Print content of db file in human readable form to stdout"}},
            {"test",           {[&](auto params) { this->test(); },                 "Test program"}},
            // tree operations
            {"print_tree",     {[&](auto params) { this->tree->print(); },          "Print all nodes of tree to stdout"}},
            {"draw_tree",      {[&](auto params) { this->tree->draw(); },           "Draw and display tree as svg picture"}},
            {"truncate_tree",  {[&](auto params) { this->tree->truncate(); },       "Remove all records, clean db file"}},
            // records operations
            {"create",         {[&](auto params) { this->create_record(params); },  "Create new record"}},
            {"remove",         {[&](auto params) { this->remove_record(params); },  "Remove record"}},
            {"update",         {[&](auto params) { this->update_record(params); },  "Update record"}},
            {"delete",         {[&](auto params) { this->delete_record(params); },  "Delete record"}},
            {"print_records",  {[&](auto params) { this->print_records(params); },  "Print all records in order by key value"}}
    };
    // @formatter:on
}


void Dbms::commandLineLoop() {
    while (true) {
        char *line = readline("> ");
        if (!line) break;
        auto lineStr = std::string(line);
        free(line);
        boost::trim(lineStr);
        if (lineStr.length() == 0) continue;
        add_history(lineStr.c_str());
        this->processInputLine(lineStr);
    }
}


void Dbms::processInputLine(std::string const &line) {
    auto cmd = line.substr(0, line.find(' '));
    auto lastWhitechar = line.rfind(' ');
    auto params = lastWhitechar == std::string::npos ? "" : line.substr(lastWhitechar);
    auto foundIter = commands.find(cmd);
    if (foundIter == commands.end()) {
        std::cout << cmd << ": command not found\n";
        return;
    }
    auto func = foundIter->second.first;
    std::invoke(func, params);
}


void Dbms::printHelp() {
    std::cout << "Available commands:\n";
    for (auto&[cmd, info] : this->commands) {
        auto&[func, desc] = info;
        std::cout << std::setw(20) <<std::left << cmd << std::setw(20) << std::left <<  desc << '\n';
    }
}


void Dbms::exit() {
    tree = nullptr;
    ::exit(0);
}

void Dbms::loadDbFile(std::string params) {

}


void Dbms::createDbFile(std::string params) {

}


void Dbms::closeDbFile() {

}


void Dbms::printDbFile() {

}


void Dbms::create_record(std::string params) {

}


void Dbms::remove_record(std::string params) {

}


void Dbms::update_record(std::string params) {

}


void Dbms::delete_record(std::string params) {

}


void Dbms::print_records(std::string params) {

}

