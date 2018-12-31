//
// Created by kamil on 10.12.18.
//

#ifndef SBD2_DBMS_HH
#define SBD2_DBMS_HH

#include <string>
#include <functional>
#include <map>
#include <memory>
#include <any>
#include <filesystem>
#include "b_plus_tree.hh"

namespace fs = std::filesystem;

class Record;


class Dbms final {
    using BTreeType = BPlusTree<int64_t, Record, 2, 3>;

public:
    static auto Main(int argc, char **argv) -> int;

private:
    inline static auto InitCommands() -> void;
    inline static auto InitAutocompletion() -> void;
    inline static auto CommandLineLoop() -> void;
    inline static auto ProcessInputLine(std::string const &line) -> void;
    // Commandline methods
    inline static auto Exit(std::string const &params = {}) -> void;
    inline static auto PrintHelp(std::string const &params = {}) -> void;
    inline static auto CreateDbFile(std::string const &params) -> void;
    inline static auto LoadDbFile(std::string const &params) -> void;
    inline static auto CloseDbFile(std::string const &params = {}) -> void;
    inline static auto PrintDbFile(std::string const &params = {}) -> void;
    inline static auto PrintTree(std::string const &params = {}) -> void;
    inline static auto DrawTree(std::string const &params = {}) -> void;
    inline static auto TruncateTree(std::string const &params = {}) -> void;
    inline static auto PrintRecords(std::string const &params = {}) -> void;
    inline static auto PrintRecordsDescending(std::string const &params = {}) -> void;
    inline static auto PrintStatistics(std::string const &params = {}) -> void;
    inline static auto LastOpStats(std::string const &params = {}) -> void;
    inline static auto LoadTestFile(std::string const &params) -> void;
    inline static auto GenTestFile(std::string const &params) -> void;
    // CRUD operations
    inline static auto CreateRecord(std::string const &params) -> void;
    inline static auto ReadRecord(std::string const &params) -> void;
    inline static auto UpdateRecord(std::string const &params) -> void;
    inline static auto DeleteRecord(std::string const &params) -> void;
    // other tools function
    inline static auto ConfirmOverridingExistingFile(fs::path const &path) -> bool;


    inline static std::map<std::string,
            std::tuple<std::function<void(std::string const &params)>, std::string>> commands;
    inline static std::unique_ptr<BTreeType> tree;
    inline static std::string prompt = "";
};


#endif //SBD2_DBMS_HH
