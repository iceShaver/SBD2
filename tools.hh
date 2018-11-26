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
template<typename _B, typename _T> constexpr inline bool instanceof() { return std::is_base_of<_B, _T>::value; }

#endif //SBD2_TOOLS_HH
