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
#include <cstring>
#include "unique_generator.hh"

std::map<std::string, std::tuple<std::function<void(std::string const &params)>, std::string>> Dbms::commands;

int Dbms::main(int argc, char **argv) {

    this->initCommands();
    this->initAutocompletion();
    this->commandLineLoop();
    this->exit();
    return 0;
}


void Dbms::test() const {
    BPlusTree<int64_t, Record, 1, 1> t("test.bin", OpenMode::CREATE_NEW);
    /*for (int i = 0; i < 1000; ++i) {
        t.createRecord(UniqueGenerator<2000, 3000>::GetRandom(), Record::Random());
    }*/
    std::cout << "Add: " << 20 << '\n';
    t.createRecord(20, Record{25, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 50 << '\n';
    t.createRecord(50, Record{12, 58, 45});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 40 << '\n';
    t.createRecord(40, Record{45, 78, 78});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 80 << '\n';
    t.createRecord(80, Record{25, 65, 12});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 70 << '\n';
    t.createRecord(70, Record{32, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 30 << '\n';
    t.createRecord(30, Record{25, 65, 98});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 10 << '\n';
    t.createRecord(10, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    t.print();
    std::cout << "Add: " << 0 << '\n';
    t.createRecord(0, Record::Random());
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 15 << '\n';
    t.createRecord(15, Record::Random());
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 39 << '\n';
    t.createRecord(39, Record::Random());
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 1000 << '\n';
    t.createRecord(1000, Record{21, 58, 69});
    t.print();
    t.draw();
    std::cout << '\n';
    std::cout << "Add: " << -1000 << '\n';
    t.createRecord(-1000, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << -555 << '\n';
    t.createRecord(-555, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 9999 << '\n';
    t.createRecord(9999, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 453 << '\n';
    t.createRecord(453, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 543 << '\n';
    t.createRecord(543, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 123 << '\n';
    t.createRecord(123, Record{25, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 12 << '\n';
    t.createRecord(12, Record{12, 58, 45});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 456 << '\n';
    t.createRecord(456, Record{45, 78, 78});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 789 << '\n';
    t.createRecord(789, Record{25, 65, 12});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 987 << '\n';
    t.createRecord(987, Record{32, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 3147 << '\n';
    t.createRecord(3147, Record{25, 65, 98});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 1654 << '\n';
    t.createRecord(1654, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 1046 << '\n';
    t.createRecord(1000546, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << -1045 << '\n';
    t.createRecord(-100045, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 4531 << '\n';
    t.createRecord(4531, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    std::cout << "Add: " << 5431 << '\n';
    t.createRecord(5431, Record{21, 58, 69});
    t.print();
    std::cout << '\n';
    t.draw();
}


void Dbms::initCommands() {
    // @formatter:off
    commands = {
            // dbms operations
            {"help",           {[this](auto const &params) -> void{ this->printHelp(); },            "Prints this help"}},
            {"exit",           {[this](auto const &params) -> void{ this->exit(); },                 "Close DB file and exit program"}},
            {"open",   {[this](auto const &params) -> void{ this->loadDbFile(params); },     "Open specified db file"}},
            {"new", {[this](auto const &params) -> void{ this->createDbFile(params); },   "Create new db file at specified location"}},
            {"close",  {[this](auto const &params) -> void{ this->closeDbFile(); },          "Save and close db file"}},
            {"print_db_file",  {[this](auto const &params) -> void{ this->printDbFile(); },          "Print content of db file in human readable form to stdout"}},
            {"test",           {[this](auto const &params) -> void{ this->test(); },                 "Test program"}},
            // tree operations
            {"print_tree",     {[this](auto const &params) -> void{ this->tree->print(); },          "Print all nodes of tree to stdout"}},
            {"draw",      {[this](auto const &params) -> void{ this->tree->draw(); },           "Draw and display tree as svg picture"}},
            {"truncate_tree",  {[this](auto const &params) -> void{ this->tree->truncate(); },       "Remove all records, clean db file"}},
            // records operations
            {"create",         {[this](auto const &params) -> void{ this->create_record(params); },  "Create new record"}},
            {"read",           {[this](auto const &params) -> void{ this->read_record(params); },    "Read record"}},
            {"update",         {[this](auto const &params) -> void{ this->update_record(params); },  "Update record"}},
            {"delete",         {[this](auto const &params) -> void{ this->delete_record(params); },  "Delete record"}},
            {"print_records",  {[this](auto const &params) -> void{ this->print_records(params); },  "Print all records in order by key value"}},
            {"stat",           {[this](auto const &params) -> void{ this->print_statistics(); },     "Print DB statistics"}}
    };
    // @formatter:on
}


void Dbms::initAutocompletion() {
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
    auto params = lastWhitechar == std::string::npos ? "" : line.substr(lastWhitechar + 1);
    auto foundIter = commands.find(cmd);
    if (foundIter == commands.end()) {
        std::cout << cmd << ": command not found\n";
        return;
    }
    auto[func, desc] = foundIter->second;
    std::invoke(func, params);
}


void Dbms::printHelp() {
    std::cout << "Available commands:\n";
    for (auto&[cmd, info] : commands) {
        auto&[func, desc] = info;
        std::cout << std::setw(20) << std::left << cmd << std::setw(20) << std::left << desc << '\n';
    }
}


void Dbms::exit() {
    tree = nullptr;
    ::exit(0);
}

void Dbms::loadDbFile(std::string const &params) {
    if (params.empty()) {
        std::cout << "You have to specify file to open\n";
        return;
    }

    if (tree) {
        std::cout << "You have to close current db before opening next\n";
        return;
    }

    try {
        tree = std::make_unique<BTreeType>(params, OpenMode::USE_EXISTING);
    } catch (std::runtime_error const &e) {
        std::cerr << "Error opening file: " << params << '\n' << e.what() << '\n';
        return;
    }
}


void Dbms::createDbFile(std::string const &params) {
    if (params.empty()) {
        std::cout << "You have to specify file to create\n";
        return;
    }
    if (tree) {
        std::cout << "You have to close current db before creating new\n";
        return;
    }
    if (fs::exists(params)) {
        std::cout << "Given file exists: " << params << "\nOverwrite? [Y/N]\n";
        std::string result;
        while (true) {
            std::getline(std::cin, result);
            if (result == "Y") break;
            if (result == "N") return;
        }
        fs::remove(params);
    }
    try {
        tree = std::make_unique<BTreeType>(params, OpenMode::CREATE_NEW);
    } catch (std::runtime_error const &e) {
        std::cerr << e.what() << '\n';
        return;
    }
}


void Dbms::closeDbFile() {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    tree = nullptr;
}


void Dbms::printDbFile() {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    std::cout << "Printing db file here!\n";
}


void Dbms::create_record(std::string const &params) {
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
        this->tree->createRecord(key, value);
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


void Dbms::read_record(std::string const &params) {
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
        auto record = this->tree->readRecord(key);
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


void Dbms::update_record(std::string const &params) {
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
        this->tree->updateRecord(key, value);
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


void Dbms::delete_record(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    if (params.empty()) {
        std::cout << "You have to specify key to delete record\n";
        return;
    }
}


void Dbms::print_records(std::string const &params) {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }

}
void Dbms::print_statistics() {
    if (!tree) {
        std::cout << "No loaded database\n";
        return;
    }
    using std::cout;
    cout << "Info & Statistics:\n";
    cout << std::setw(40) << std::left << "DB file: " << fs::absolute(this->tree->filePath) << '\n';
    cout << std::setw(40) << std::left << "DB file size: " << fs::file_size(this->tree->filePath) << " bytes\n";
    cout << std::setw(40) << std::left << "Underlying data type: " << this->tree->name() << '\n';
    cout << std::setw(40) << std::left << "Inner node degree: " << this->tree->innerNodeDegree() << '\n';
    cout << std::setw(40) << std::left << "Leaf node degree: " << this->tree->leafNodeDegree() << '\n';
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



