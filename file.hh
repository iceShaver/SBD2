//
// Created by kamil on 12.12.18.
//

#ifndef SBD2_FILE_HH
#define SBD2_FILE_HH


#include <fstream>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

class File final {
public:
    File() = default;
    File(fs::path const &path, std::ios::openmode const &mode,
         std::function<void(void)> incReadsCntCallback,
         std::function<void(void)> incWritesCntCallback);

    template<typename T> void write(size_t offset, T const &data);

    void write(size_t offset, std::vector<char> const &data);

    template<typename T> T read(size_t offset);

    std::vector<char> read(size_t offset, size_t size);

    void clear() { this->fileHandle.clear(); }
    bool bad() { return this->fileHandle.bad(); }
    bool good() { return this->fileHandle.good(); }
    auto tellg() { return this->fileHandle.tellg(); }
    auto tellp() { return this->fileHandle.tellp(); }
    bool eof() { return this->fileHandle.eof(); }


private:
    std::fstream fileHandle;
    std::function<void(void)> incReadsCntCallback;
    std::function<void(void)> incWritesCntCallback;


};

template<typename T>
void File::write(size_t offset, T const &data) {
    auto dataVector = std::vector<char>();
    dataVector.resize(sizeof(T));
    *reinterpret_cast<T *>(dataVector.data()) = data;
    this->write(offset, dataVector);
}

template<typename T>
T File::read(size_t offset) {
    auto size = sizeof(T);
    auto result = this->read(offset, size);
    return *reinterpret_cast<T *>(result.data()); // TODO: possibly wrong????}

}
#endif //SBD2_FILE_HH
