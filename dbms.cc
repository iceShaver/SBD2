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
    //CommandLineLoop();
    Test();
    Exit();
    return 0;
}


void Dbms::Test(std::string const &params) {
    BPlusTree<int64_t, Record, 1, 1> t("Test.bin", OpenMode::CREATE_NEW);
    /*for (int i = 0; i < 1000; ++i) {
        t.createRecord(UniqueGenerator<2000, 3000>::getRandom(), Record::Random());
    }*/
    // @formatter:off
    std::cout<<"Add: "<<20<<'\n';t.createRecord(20,Record{25,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<50<<'\n';t.createRecord(50,Record{12,58,45});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<40<<'\n';t.createRecord(40,Record{45,78,78});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<80<<'\n';t.createRecord(80,Record{25,65,12});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<70<<'\n';t.createRecord(70,Record{32,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<30<<'\n';t.createRecord(30,Record{25,65,98});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<10<<'\n';t.createRecord(10,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<0<<'\n'; t.createRecord(00,Record::Random());t.print();std::cout<<'\n';
    std::cout<<"Add: "<<15<<'\n';t.createRecord(15,Record::Random());t.print();std::cout<<'\n';
    std::cout<<"Add: "<<39<<'\n';t.createRecord(39,Record::Random());t.print();std::cout<<'\n';
    std::cout<<"Add: "<<1000<<'\n';t.createRecord(1000,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<-1000<<'\n';t.createRecord(-1000,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<-555<<'\n';t.createRecord(-555,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<9999<<'\n';t.createRecord(9999,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<453<<'\n';t.createRecord(453,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<543<<'\n';t.createRecord(543,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<123<<'\n';t.createRecord(123,Record{25,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<12<<'\n';t.createRecord(12,Record{12,58,45});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<456<<'\n';t.createRecord(456,Record{45,78,78});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<789<<'\n';t.createRecord(789,Record{25,65,12});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<987<<'\n';t.createRecord(987,Record{32,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<3147<<'\n';t.createRecord(3147,Record{25,65,98});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<1654<<'\n';t.createRecord(1654,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<1046<<'\n';t.createRecord(1000546,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<-1045<<'\n';t.createRecord(-100045,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<4531<<'\n';t.createRecord(4531,Record{21,58,69});t.print();std::cout<<'\n';
    std::cout<<"Add: "<<5431<<'\n';t.createRecord(5431,Record{21,58,69});t.print();std::cout<<'\n';
    t.draw();
    // @formatter:on
    for(auto[k, v] : t){
        std::cout << k << " " << v << '\n';
    }
    /*auto it = t.begin();
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
    ++it;
    std::cout << (*it).first << " " << (*it).second << '\n';
*/

    std::cout << "success\n";
}


void Dbms::InitCommands() {
    // @formatter:off
    commands = {
            // dbms operations
            {"help",           {PrintHelp,          "Prints this help"}},
            {"exit",           {Exit,               "Close DB file and Exit program"}},
            {"open",           {LoadDbFile,         "Open specified db file"}},
            {"new",            {CreateDbFile,       "Create new db file at specified location"}},
            {"close",          {CloseDbFile,        "Save and close db file"}},
            {"printdbfile",    {PrintDbFile,        "Print content of db file in human readable form to stdout"}},
            {"test",           {Test,               "Test program"}},
            // tree operations
            {"printtree",      {PrintTree,          "Print all nodes of tree to stdout"}},
            {"draw",           {DrawTree,           "Draw and display tree as svg picture"}},
            {"truncatetree",   {TruncateTree,       "Remove all records, clean db file"}},
            // records operations
            {"create",         {CreateRecord,       "Create new record"}},
            {"read",           {ReadRecord,         "Read record"}},
            {"update",         {UpdateRecord,       "Update record"}},
            {"delete",         {DeleteRecord,       "Delete record"}},
            {"printrecords",   {PrintRecords,       "Print all records in order by key value"}},
            {"stats",          {PrintStatistics,    "Print DB statistics"}},
            {"testfile",       {LoadTestFile,       "Load test file"}},
            {"gentestfile",    {GenTestFile,        "Generate random test file with specified size (if not size is random"}}
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
        char *line = readline((prompt + "> ").c_str());
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
        std::cout << "No loaded database\n";
        return;
    }
    tree = nullptr;
    prompt = "";
}


void Dbms::PrintDbFile(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    std::cout << "Printing db file:\n";
    tree->printFile();
}


void Dbms::CreateRecord(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify records data: key grade1 grade2 grade3\n";
        return;
    }
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
        std::cout << "No loaded database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify key to read record\n";
        return;
    }

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
        std::cout << "No loaded database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify records data: key grade1 grade2 grade3\n";
        return;
    }
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
        std::cout << "Error while adding record:\n";
        std::cout << e.what() << '\n';
        return;
    }
}


void Dbms::DeleteRecord(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify key to delete record\n";
        return;
    }
}


void Dbms::PrintRecords(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
}
void Dbms::PrintStatistics(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    using std::cout;
    cout << "Info & Statistics:\n";
    cout << std::setw(40) << std::left << "DB file: " << fs::absolute(tree->filePath) << '\n';
    cout << std::setw(40) << std::left << "DB file size: " << fs::file_size(tree->filePath) << " bytes\n";
    cout << std::setw(40) << std::left << "Underlying data type: " << tree->name() << '\n';
    cout << std::setw(40) << std::left << "Inner node degree: " << tree->innerNodeDegree() << '\n';
    cout << std::setw(40) << std::left << "Leaf node degree: " << tree->leafNodeDegree() << '\n';
    cout << std::setw(40) << std::left << "Max nodes in RAM simultaneously: " << BTreeType::ANode::GetMaxNodesCount() << '\n';
    cout << std::setw(40) << std::left << "Current nodes in RAM: " << BTreeType::ANode::GetCurrentNodesCount() << '\n';
    /*cout << std::setw(40) << std::left << "Records number: " << this->tree->recordsNumber() << '\n';
    cout << std::setw(40) << std::left << "Nodes number: " << this->tree->nodesNumber() << '\n';
    cout << std::setw(40) << std::left << "Disk reads count (current session): " << this->tree->diskReadsCount() << '\n';
    cout << std::setw(40) << std::left << "Disk writes count (current session): " << this->tree->diskWritesCount() << '\n';
    cout << std::setw(40) << std::left << "Disk reads count (last operation): " << this->tree->lastOpDiskReadsCount() << '\n';
    cout << std::setw(40) << std::left << "Disk writes count (last operation): " << this->tree->lastOpDiskWritesCount() << '\n';
    cout << std::setw(40) << std::left << "Disk total I/O: "
         << this->tree->diskWritesCount() + this->tree->diskReadCount() << '\n';
    cout << std::setw(40) << std::left << "Disk Memory utilization: " << this->diskMemoryUtilization() << " %\n";
    cout << std::setw(40) << std::left << "RAM usage: " << this->ramUsage() << " %\n";*/




}


void Dbms::LoadTestFile(std::string const &params) {
    if (params.empty()) {
        std::cout << "Missing parameters: filename\n";
        return;
    }

    auto filePath = fs::path(params);
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
        fileHandle << "create " << uniqueGenerator.getRandom() << ' '
                   << +randomRecord.get_grade(1) << ' '
                   << +randomRecord.get_grade(2) << ' '
                   << +randomRecord.get_grade(3) << '\n';
    }
}


void Dbms::PrintTree(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    Dbms::tree->print();
    std::cout << '\n';
}


void Dbms::DrawTree(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    Dbms::tree->draw();
}


void Dbms::TruncateTree(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
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




