//
// Created by kamil on 25.11.18.
//

#ifndef SBD2_TOOLS_HH
#define SBD2_TOOLS_HH

#include <iostream>
#include <functional>
#include <cxxabi.h>
#include <random>
#include <filesystem>

namespace Tools {
    namespace Terminal {
        enum class Color{
            FG_RED = 31,
            FG_GREEN = 32,
            FG_BLUE = 34,
            FG_DEFAULT = 39,
            BG_RED = 41,
            BG_GREEN = 42,
            BG_BLUE = 44,
            BG_DEFAULT = 49
        };
        inline static void set_color(Color color){
            std::cout << "\033[" << static_cast<int>(color) << 'm';
        }
        inline static std::string getColorString(Color color){
            return "\033[" + std::to_string(static_cast<int>(color)) + 'm';
        }

    }
    namespace fs = std::filesystem;
    struct Config {
        inline static bool verboseMode = false;
        inline static int debugLevel = 0;
    };

    template<typename TBase, typename TDerived>
    constexpr inline bool instanceof([[maybe_unused]] TBase &b, [[maybe_unused]] TDerived &t) {
        return std::is_base_of<TDerived, TBase>::value;
    }

    template<typename _F> constexpr inline static void verbose(_F f) { if (Config::verboseMode) std::invoke(f); }
    template<typename _F> constexpr inline static void debug(_F f, size_t level = 1) {
        if (Config::debugLevel >= level)
            std::invoke(f);
    }

    template<typename T> std::string typeName() {
        std::unique_ptr<char, void (*)(void *)> t(abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
                                                  std::free);
        return t ? t.get() : typeid(T).name();
    }

    template<typename T>
    T random(T min, T max){
        std::random_device rd{};
        auto gen = std::mt19937_64{rd()};
        std::uniform_int_distribution<T> uid{min, max};
        return uid(gen);
    }


    /**
     * Return true with probability given in parameter <0 - 1>
     * @param p
     * @return
     */
    inline static bool probability(double p){
        uint64_t precision = 1'000'000;
        std::random_device rd{};
        auto gen = std::mt19937_64{rd()};
        std::uniform_int_distribution<uint64_t> uid{0, precision};
        auto random =  uid(gen);
        bool result = precision * p > random;
        return result;
    }


}
#endif //SBD2_TOOLS_HH
