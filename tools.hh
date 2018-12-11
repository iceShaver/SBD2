//
// Created by kamil on 25.11.18.
//

#ifndef SBD2_TOOLS_HH
#define SBD2_TOOLS_HH

#include <iostream>
#include <functional>
#include <cxxabi.h>

namespace Tools {
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
}
#endif //SBD2_TOOLS_HH
