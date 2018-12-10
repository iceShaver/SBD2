//
// Created by kamil on 25.11.18.
//

#ifndef SBD2_TOOLS_HH
#define SBD2_TOOLS_HH

#include <iostream>
#include <functional>

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
}
#endif //SBD2_TOOLS_HH
