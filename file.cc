

//
// Created by kamil on 12.12.18.
//

#include "file.hh"


File::File(fs::path const &path, std::ios::openmode const &mode,
           std::function<void(void)> incReadsCntCallback, std::function<void(void)> incWritesCntCallback) {
    this->fileHandle.open(path, mode);
    this->incReadsCntCallback = std::move(incReadsCntCallback);
    this->incWritesCntCallback = std::move(incWritesCntCallback);
    this->fileHandle.rdbuf()->pubsetbuf(0, 0);
}




void File::write(size_t offset, std::vector<char> const &data) {
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




std::vector<char> File::read(size_t offset, size_t size) {
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
