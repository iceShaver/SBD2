//
// Created by kamil on 10.12.18.
//

#ifndef SBD2_UNIQUE_GENERATOR_HH
#define SBD2_UNIQUE_GENERATOR_HH

#include <set>
#include <random>

template<typename T>
class UniqueGenerator final {
    using NumberType = int64_t;
public:
    UniqueGenerator(T min, T max) : uid{min, max}, min(min), max(max) {}
    T getRandom();
    std::set<NumberType> &getDrawedNumbers() { return drawedNumbers; }
private:
    std::set<NumberType> drawedNumbers;
    std::random_device rd{};
    std::mt19937_64 gen = std::mt19937_64{rd()};
    std::uniform_int_distribution<T> uid;
    T min, max;
};


template<typename T>
T UniqueGenerator<T>::getRandom() {
    if (max - min == drawedNumbers.size() - 1)// TODO: fix overflow
        throw std::runtime_error("Generator is out of unique Random numbers");
    while (true) {
        auto drawedNumber = uid(gen);
        if (drawedNumbers.find(drawedNumber) == drawedNumbers.end()) {
            drawedNumbers.insert(drawedNumber);
            return drawedNumber;
        }
    }
}

#endif //SBD2_UNIQUE_GENERATOR_HH
