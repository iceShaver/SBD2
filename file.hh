//
// Created by kamil on 12.12.18.
//

#ifndef SBD2_FILE_HH
#define SBD2_FILE_HH


#include <fstream>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

template<typename TReadsCallback, typename TWritesCallback>
class File final {
public:
    File(fs::path const &path, std::ios::openmode const &mode,
         TReadsCallback incReadsCntCallback,
         TWritesCallback incWritesCntCallback);

    template<typename T>
    void write(size_t offset, T const &data);

    void write(size_t offset, std::vector<char> const &data);

    template<typename T>
    T read(size_t offset);

    std::vector<char> read(size_t offset, size_t size);

    void clear() { this->fileHandle.clear(); }
private:

    std::fstream fileHandle;
    TReadsCallback incReadsCntCallback;
    TWritesCallback incWritesCntCallback;


};

template<typename TReadsCallback, typename TWritesCallback>
File<TReadsCallback, TWritesCallback>::File(fs::path const &path, std::ios::openmode const &mode,
                                            TReadsCallback incReadsCntCallback, TWritesCallback incWritesCntCallback) {
    this->fileHandle.open(path, mode);
    this->incReadsCntCallback = std::move(incReadsCntCallback);
    this->incWritesCntCallback = std::move(incWritesCntCallback);
    this->fileHandle.rdbuf()->pubsetbuf(0, 0);
}


template<typename TReadsCallback, typename TWritesCallback>
template<typename T>
void File<TReadsCallback, TWritesCallback>::write(size_t offset, T const &data) {
    auto dataVector = std::vector<char>();
    dataVector.resize(sizeof(T));
    *reinterpret_cast<T *>(dataVector.data()) = data;
    this->write(offset, dataVector);
}


template<typename TReadsCallback, typename TWritesCallback>
void File<TReadsCallback, TWritesCallback>::write(size_t offset, std::vector<char> const &data) {
    if (this->fileHandle.bad()) {
        throw std::runtime_error(
                "Disk write at offset" + std::to_string(offset) + " of size " + std::to_string(data.size()) +
                " failed before");
    }
    std::invoke(this->incWritesCntCallback);
    this->fileHandle.clear();
    this->fileHandle.seekp(offset);
    this->fileHandle.write(data.data(), data.size());
    if (this->fileHandle.bad())
        throw std::runtime_error(
                "Disk write at offset" + std::to_string(offset) + " of size " + std::to_string(data.size()) +
                " failed after");
}


template<typename TReadsCallback, typename TWritesCallback>
template<typename T>
T File<TReadsCallback, TWritesCallback>::read(size_t offset) {
    auto size = sizeof(T);
    auto result = this->read(offset, size);
    return *reinterpret_cast<T *>(result.data()); // TODO: possibly wrong????}


template<typename TReadsCallback, typename TWritesCallback>
std::vector<char> File<TReadsCallback, TWritesCallback>::read(size_t offset, size_t size) {
    if (this->fileHandle.bad()) {
        throw std::runtime_error(
                "Disk read at offset" + std::to_string(offset) + " of size " + std::to_string(size) + " failed before");
    }
    std::invoke(this->incReadsCntCallback);
    this->fileHandle.clear();
    std::vector<char> result;
    result.resize(size);
    this->fileHandle.seekg(offset);
    this->fileHandle.read(result.data(), size);
    if (this->fileHandle.bad())
        throw std::runtime_error(
                "Disk read at offset" + std::to_string(offset) + " of size " + std::to_string(size) + " failed after");
    return result;
}


/*
File::File(fs::path const &path, std::ios::openmode const &mode, std::function<void(void)> incReadsCntCallback,
           std::function<void(void)> incWritesCntCallback) {


}


template<typename T>
void File::write(size_t offset, T const &data) {

}


void File::write(size_t offset, std::vector<char> const &data) {

}


template<typename T>
T File::read(size_t offset) {

}


std::vector<char> File::read(size_t offset, size_t size) {

}*/
#endif //SBD2_FILE_HH
