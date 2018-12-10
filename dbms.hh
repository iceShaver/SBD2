//
// Created by kamil on 10.12.18.
//

#ifndef SBD2_DBMS_HH
#define SBD2_DBMS_HH

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <any>
#include <filesystem>
#include "b_plus_tree.hh"
namespace fs = std::filesystem;

class Record;
//template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree> class BPlusTree;


class Dbms final {
    using BTreeType = BPlusTree<int64_t, Record, 3, 2>;
public:
    int main(int argc, char **argv);

private:
    void test() const;
    void initCommands();
    void commandLineLoop();
    void processInputLine(std::string const &line);
    void printHelp();
    void exit();

    void loadDbFile(std::string const&params);
    void createDbFile(std::string const&params);
    void closeDbFile();
    void printDbFile();
    void create_record(std::string const&params);
    void remove_record(std::string const&params);
    void update_record(std::string const&params);
    void delete_record(std::string const&params);
    void print_records(std::string const&params);


    std::unordered_map<std::string, std::pair<std::function<void(std::string)>, std::string>> commands;
    std::unique_ptr<BTreeType> tree;



};


#endif //SBD2_DBMS_HH
