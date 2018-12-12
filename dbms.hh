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
//template<typename TKey, typename TValue, size_t TInnerNodeDegree, size_t TLeafNodeDegree> class BPlusTree;


class Dbms final {
    using BTreeType = BPlusTree<int64_t, Record, 1, 2>;

public:
    static int Main(int argc, char **argv);

private:
    inline static void InitCommands();
    inline static void InitAutocompletion();
    inline static void CommandLineLoop();
    inline static void ProcessInputLine(std::string const &line);

    // Commandline methods
    inline static void Test(std::string const &params = {});
    inline static void Exit(std::string const &params = {});
    inline static void PrintHelp(std::string const &params = {});
    inline static void CreateDbFile(std::string const &params);
    inline static void LoadDbFile(std::string const &params);
    inline static void CloseDbFile(std::string const &params = {});
    inline static void PrintDbFile(std::string const &params = {});
    inline static void PrintTree(std::string const &params = {});
    inline static void DrawTree(std::string const &params = {});
    inline static void TruncateTree(std::string const &params = {});
    inline static void PrintRecords(std::string const &params = {});
    inline static void PrintStatistics(std::string const &params = {});
    inline static void LoadTestFile(std::string const &params);
    inline static void GenTestFile(std::string const &params);
    // CRUD operations
    inline static void CreateRecord(std::string const &params);
    inline static void ReadRecord(std::string const &params);
    inline static void UpdateRecord(std::string const &params);
    inline static void DeleteRecord(std::string const &params);


    // other tools function
    inline static bool ConfirmOverridingExistingFile(fs::path const &path);
    // data
    inline static std::map<std::string,
            std::tuple<std::function<void(std::string const &params)>, std::string>> commands;
    inline static std::unique_ptr<BTreeType> tree;

};


#endif //SBD2_DBMS_HH
