//
// Created by kamil on 25.11.18.
//

#ifndef SBD2_TOOLS_HH
#define SBD2_TOOLS_HH

#include <iostream>
#include <functional>
#include "Config.hh"

template<typename _F> constexpr inline void verbose(_F f) { if (Config::verbose) std::invoke(f); }
template<typename _F> constexpr inline void debug(_F f, size_t level = 1) {
    if (Config::debugLevel >= level)
        std::invoke(f);
}

template<typename TBase, typename TDerived>
constexpr inline bool instanceof([[maybe_unused]] TBase &b, [[maybe_unused]] TDerived &t) {
    return std::is_base_of<TDerived, TBase>::value;
}

#endif //SBD2_TOOLS_HH
