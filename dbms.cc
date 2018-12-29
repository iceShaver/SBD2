//
// Created by kamil on 10.12.18.
//

#include "dbms.hh"
#include "b_plus_tree.hh"
#include "unique_generator.hh"
#include <readline/readline.h>
#include <readline/history.h>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <functional>
#include <cstring>


int Dbms::Main(int argc, char **argv) {

    InitCommands();
    InitAutocompletion();
    CommandLineLoop();
    Exit();
    return 0;
}


void Dbms::InitCommands() {
    // @formatter:off
    commands = {
            // dbms operations
            {"help",           {PrintHelp,              "Prints this help"}},
            {"exit",           {Exit,                   "Close DB file and Exit program"}},

            {"open",           {LoadDbFile,             "Open specified db file"}},
            {"new",            {CreateDbFile,           "Create new db file at specified location"}},
            {"close",          {CloseDbFile,            "Save and close db file"}},

            {"file",           {PrintDbFile,            "Print content of db file in human readable form to stdout"}},
            {"nodes",          {PrintTree,              "Print all nodes of tree to stdout"}},
            {"draw",           {DrawTree,               "Draw and display tree as svg picture"}},
            {"ls",             {PrintRecords,           "Print all records in order by key value"}},
            {"lsd",            {PrintRecordsDescending, "Print all records in order by key value (descending)"}},

            {"load",           {LoadTestFile,           "Load test file"}},
            {"gentestfile",    {GenTestFile,            "Generate random test file with specified size (if not size is random"}},
            // tree operations


            {"truncatetree",   {TruncateTree,           "Remove all records, clean db file"}},
            // records operations
            {"create",         {CreateRecord,           "Create new record"}},
            {"read",           {ReadRecord,             "Read record"}},
            {"update",         {UpdateRecord,           "Update record"}},
            {"delete",         {DeleteRecord,           "Delete record"}},


            {"stats",          {PrintStatistics,        "Print DB statistics"}},
            {"lastop",         {LastOpStats,            "Last operation statistics"}}
    };
    // @formatter:on
}


void Dbms::InitAutocompletion() {
    // set quote characters
    rl_completer_quote_characters = "\"'";

    // set function responsible for autocomplete
    rl_attempted_completion_function = [](const char *text, int start, int end) -> char ** {
        // if not beginning of command then don't complete
        if (start != 0) return nullptr;
        // return array of matches followed by NULL, for that use below function, which supply with
        // function for returning succesive match or NULL if not more matches
        return rl_completion_matches(text, [](const char *text, int state) -> char * {
            static std::vector<char *> matches;
            static std::pair<std::vector<char *>::iterator, std::vector<char *>::iterator> matchesRange;
            if (!state) {
                matches.clear();
                for (auto&[cmd, desc] : Dbms::commands) {
                    int cmpResult = strncmp(text, cmd.c_str(), strlen(text));
                    if (cmpResult > 0) continue;
                    if (cmpResult < 0) break;
                    // every element will be freed by GNU readline library
                    auto match = matches.emplace_back(new char[cmd.length() + 1]);
                    strcpy(match, cmd.c_str());
                }
                matchesRange.first = matches.begin();
                matchesRange.second = matches.end();
            }
            return matchesRange.first == matchesRange.second ? nullptr : *matchesRange.first++;
        });
    };
}


void Dbms::CommandLineLoop() {
    while (true) {
        char *line = readline((Tools::Terminal::getColorString(Tools::Terminal::Color::FG_GREEN) + prompt + "> " +
                               Tools::Terminal::getColorString(Tools::Terminal::Color::FG_DEFAULT)).c_str());
        if (!line) break;
        auto lineStr = std::string(line);
        free(line);
        boost::trim(lineStr);
        if (lineStr.length() == 0) continue;
        add_history(lineStr.c_str());
        ProcessInputLine(lineStr);
    }
    std::cout << '\n';
}


void Dbms::ProcessInputLine(std::string const &line) {
    auto cmd = line.substr(0, line.find(' '));
    if(cmd[0]=='#')
        return;
    auto firstWhitechar = line.find(' ');
    auto params = firstWhitechar == std::string::npos ? "" : line.substr(firstWhitechar + 1);
    auto foundIter = commands.find(cmd);
    if (foundIter == commands.end()) {
        std::cout << cmd << ": command not found\n";
        return;
    }
    auto[func, desc] = foundIter->second;
    std::invoke(func, params);
}


void Dbms::PrintHelp(std::string const &params) {
    std::cout << "Available commands:\n";
    for (auto&[cmd, info] : commands) {
        auto&[func, desc] = info;
        std::cout << std::setw(20) << std::left << cmd << std::setw(20) << std::left << desc << '\n';
    }
}


void Dbms::Exit(std::string const &params) {
    std::cout << "Exiting...\n";
    tree = nullptr;
    ::exit(0);
}

void Dbms::LoadDbFile(std::string const &params) {
    if (tree) {
        std::cout << "You have to close current db before opening next\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify file to open\n";
        return;
    }


    try {
        tree = std::make_unique<BTreeType>(params, OpenMode::USE_EXISTING);
    } catch (std::runtime_error const &e) {
        std::cerr << "Error opening file: " << params << '\n' << e.what() << '\n';
        return;
    }
    prompt = params;
}


void Dbms::CreateDbFile(std::string const &params) {
    if (tree) {
        std::cout << "You have to close current db before creating new\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify file to create\n";
        return;
    }

    if (!ConfirmOverridingExistingFile(params))
        return;

    try {
        tree = std::make_unique<BTreeType>(params, OpenMode::CREATE_NEW);
    } catch (std::runtime_error const &e) {
        std::cerr << e.what() << '\n';
        return;
    }
    prompt = params;
}


void Dbms::CloseDbFile(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree = nullptr;
    prompt = "";
}


void Dbms::PrintDbFile(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    std::cout << "Printing db file:\n";
    tree->printFile();
}


void Dbms::CreateRecord(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify records data: key grade1 grade2 grade3\n";
        return;
    }
    tree->resetOpCounters();
    try {
        auto keyToken = params.substr(0, params.find(' '));
        auto recordToken = params.substr(params.find(' ') + 1);
        auto value = Record(recordToken);
        auto key = std::stoll(keyToken);
        tree->createRecord(key, value);
    } catch (std::out_of_range const &e) {
        std::cout << "Invalid arguments: " << params << '\n';
        std::cout << e.what() << '\n';
        return;
    } catch (std::invalid_argument const &e) {
        std::cout << "Invalid arguments: " << params << '\n';
        std::cout << e.what() << '\n';
        return;
    } catch (std::runtime_error const &e) {
        std::cout << "Error while adding record:\n";
        std::cout << e.what() << '\n';
        return;
    }
}


void Dbms::ReadRecord(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify key to read record\n";
        return;
    }
    tree->resetOpCounters();
    try {
        auto key = std::stoll(params);
        auto record = tree->readRecord(key);
        if (record)
            std::cout << *record << '\n';
        else
            std::cout << "Record with key: " << key << "not found\n";

    } catch (std::invalid_argument const &e) {
        std::cout << "Invalid arguments: " << params << '\n';
        std::cout << e.what() << '\n';
        return;
    } catch (std::runtime_error const &e) {
        std::cout << "Error while finding record:\n";
        std::cout << e.what() << '\n';
        return;
    }
}


void Dbms::UpdateRecord(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify records data: key grade1 grade2 grade3\n";
        return;
    }
    tree->resetOpCounters();
    try {
        auto keyToken = params.substr(0, params.find(' '));
        auto recordToken = params.substr(params.find(' ') + 1);
        auto value = Record(recordToken);
        auto key = std::stoll(keyToken);
        tree->updateRecord(key, value);
    } catch (std::out_of_range const &e) {
        std::cout << "Invalid arguments: " << params << '\n';
        std::cout << e.what() << '\n';
        return;
    } catch (std::invalid_argument const &e) {
        std::cout << "Invalid arguments: " << params << '\n';
        std::cout << e.what() << '\n';
        return;
    } catch (std::runtime_error const &e) {
        std::cout << "Error while updating record:\n";
        std::cout << e.what() << '\n';
        return;
    }
}


void Dbms::DeleteRecord(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree->resetOpCounters();
    if (params.empty()) {
        std::cout << "You have to specify key to delete record\n";
        return;
    }
    try {
        auto key = std::stoll(params);
        tree->deleteRecord(key);
    }catch (std::invalid_argument const &e){
        std::cout << "Invalid arguments: " << params << '\n';
        std::cout << e.what() << '\n';
        return;
    }catch (std::runtime_error const &e) {
        std::cout << "Error while deletion of record: " + params + '\n';
        std::cout << e.what() << '\n';
        return;
    }



}


void Dbms::PrintRecords(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree->resetOpCounters();
    std::cout << "Key:\tValue:\n";
    auto nc = BTreeType::ANode::GetMaxNodesCount();
    int count = 0;
    for (auto[k, v] : *tree) {
        auto nc = BTreeType::ANode::GetMaxNodesCount();
        std::cout << k << '\t' << v << '\n';
        ++count;
    }
    std::cout << "Total count: " << count << " records\n";
}
void Dbms::PrintRecordsDescending(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree->resetOpCounters();
    std::cout << "Key:\tValue:\n";
    int count = 0;
    for (auto it = tree->rbegin(); it != tree->rend(); ++it) {
        auto[k, v] = *it;
        std::cout << k << '\t' << v << '\n';
        ++count;
    }
    std::cout << "Total count: " << count << " records\n";
}

void Dbms::PrintStatistics(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    using std::cout;
    tree->disableCounters();
    cout << std::setw(40) << std::left << "DB file: " << fs::absolute(tree->filePath) << '\n';
    cout << std::setw(40) << std::left << "DB file size: " << fs::file_size(tree->filePath) << " bytes\n";
    cout << std::setw(40) << std::left << "Underlying data type: " << tree->name() << '\n';
    cout << std::setw(40) << std::left << "Node degree:" << "Inner: " << tree->innerNodeDegree() << " Leaf: "
         << tree->leafNodeDegree() << '\n';
    cout << std::setw(40) << std::left << "Tree height: " << tree->getHeight() << '\n';
    cout << std::setw(40) << std::left << "Nodes in RAM: " << "Max: "
         << BTreeType::ANode::GetMaxNodesCount() << " Current: " << BTreeType::ANode::GetCurrentNodesCount() << "\n";
    cout << std::setw(40) << std::left << "Records number: " << tree->getRecordsNumber() << '\n';
    auto[innerNodesCount, leafNodesCount] = tree->getNodesCount();
    cout << std::setw(40) << std::left << "Nodes number: " << "Inner: " << innerNodesCount << " Leaf: "
         << leafNodesCount
         << " Sum: " << innerNodesCount + leafNodesCount << '\n';

    cout << std::setw(40) << std::left << "Disk IO (session): " << "R: " << tree->getSessionDiskReadsCout() << " W: "
         << tree->getSessionDiskWritesCount() << " Sum: "
         << tree->getSessionDiskReadsCout() + tree->getSessionDiskWritesCount() << '\n';
    cout << std::setw(40) << std::left << "Disk Memory utilization: " << tree->getDiskUtilizationPercent() << " %\n";
    //cout << std::setw(40) << std::left << "RAM usage: " << this->ramUsage() << " %\n";
    tree->enableCounters();
}


void Dbms::LoadTestFile(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }

    if (params.empty()) {
        std::cout << "Missing parameters: filename\n";
        return;
    }
    tree->resetOpCounters();
    auto filePath = fs::path(params);
    if(!fs::exists(filePath)){
        std::cout << "File: " << fs::absolute(filePath) << " does not exist\n";
        return;
    }
    auto fileHandle = std::ifstream(filePath, std::ios::in);
    if (fileHandle.bad()) {
        std::cout << "Unable to open file: " << fs::absolute(filePath) << '\n';
        return;
    }
    auto line = std::string();
    while (!std::getline(fileHandle, line).eof()) {
        ProcessInputLine(line);
    }
}


void Dbms::GenTestFile(std::string const &params) {
    if (params.empty()) {
        std::cout << "Missing parameters: filename size\n";
        return;
    }
    std::vector<std::string> tokens;
    boost::split(tokens, params, boost::is_any_of(" "));
    uint64_t size = Tools::random(0ull, 10000ull);
    fs::path filePath;
    try {
        filePath = tokens[0];
        if (tokens.size() == 2)
            size = std::stoull(tokens[1]);
    } catch (std::invalid_argument const &e) {
        std::cout << "Bad size argument: " << tokens[1] << '\n';
        return;
    } catch (std::out_of_range const &e) {
        std::cout << "Invalid arguments, should be [filename] [size]\n";
        return;
    }
    if (!ConfirmOverridingExistingFile(filePath))
        return;
    auto fileHandle = std::ofstream(filePath, std::ios::trunc | std::ios::out);
    if (fileHandle.bad()) {
        std::cout << "Unable to open file: " << fs::absolute(filePath) << '\n';
        return;
    }
    auto uniqueGenerator = UniqueGenerator(uint64_t(0), 10 * size);
    auto ops = {"create", "update", "delete"};
    for (int i = 0; i < size; ++i) {
        auto randomRecord = Record::Random();
        if (Tools::probability(0.10) && uniqueGenerator.getDrawedNumbers().size()) {
            auto index = Tools::random(0ul, uniqueGenerator.getDrawedNumbers().size() - 1);
            auto it = uniqueGenerator.getDrawedNumbers().begin();
            std::advance(it, index);
            auto key = *it;
            fileHandle << "update " << key << ' '
                       << +randomRecord.get_grade(1) << ' '
                       << +randomRecord.get_grade(2) << ' '
                       << +randomRecord.get_grade(3) << '\n';
        } else {
            fileHandle << "create " << uniqueGenerator.getRandom() << ' '
                       << +randomRecord.get_grade(1) << ' '
                       << +randomRecord.get_grade(2) << ' '
                       << +randomRecord.get_grade(3) << '\n';
        }

    }
}


void Dbms::PrintTree(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree->resetOpCounters();
    Dbms::tree->print();
    std::cout << '\n';
}


void Dbms::DrawTree(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree->resetOpCounters();
    Dbms::tree->draw();
}


void Dbms::TruncateTree(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    tree = std::make_unique<BTreeType>(tree->filePath, OpenMode::CREATE_NEW);
}


bool Dbms::ConfirmOverridingExistingFile(fs::path const &path) {
    if (fs::exists(path)) {
        std::cout << "Given file exists: " << fs::absolute(path) << "\nOverwrite? [Y/N]: ";
        std::string result;
        while (true) {
            std::getline(std::cin, result);
            if (result == "Y" || result == "y") break;
            if (result == "N" || result == "n") return false;
        }
        fs::remove(path);
    }
    return true;
}


void Dbms::LastOpStats(std::string const &params) {
    if (!tree) {
        std::cout << "No opened database\n";
        return;
    }
    std::cout << "Disk reads:\t" << tree->getCurrentOperationDiskReadsCount() << '\n';
    std::cout << "Disk writes:\t" << tree->getCurrentOperationDiskWritesCount() << '\n';
}





