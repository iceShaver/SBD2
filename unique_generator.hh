//
// Created by kamil on 10.12.18.
//

#ifndef SBD2_UNIQUE_GENERATOR_HH
#define SBD2_UNIQUE_GENERATOR_HH

#include <set>
#include <random>

template <int64_t TMin, int64_t TMax>
class UniqueGenerator final{
    using NumberType = int64_t;
public:
    static NumberType GetRandom();
private:
    inline static std::set<NumberType> drawedNumbers;
    inline static std::random_device rd{};
    inline static std::mt19937_64 gen = std::mt19937_64{rd()};
    inline static std::uniform_int_distribution<NumberType> uid{TMin, TMax};
};

template<int64_t TMin, int64_t TMax>
typename UniqueGenerator<TMin, TMax>::NumberType UniqueGenerator<TMin, TMax>::GetRandom() {
    if(TMax - TMin == drawedNumbers.size() - 1)
        throw std::runtime_error("Generator is out of unique Random numbers");
    while(true) {
        auto drawedNumber = uid(gen);
        if(drawedNumbers.find(drawedNumber) == drawedNumbers.end()){
            drawedNumbers.insert(drawedNumber);
            return drawedNumber;
        }
    }
}


#endif //SBD2_UNIQUE_GENERATOR_HH
