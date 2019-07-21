#pragma once

#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <utility>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <map>
#include <set>

#include "io.h"

template< typename T >
constexpr T infty() {
    return std::numeric_limits< T >::infinity();
}

template< typename T >
constexpr T pi = T(3.1415926535897932385);

template< typename T >
T clamp(const T low, const T high, const T val) {
    return std::min(high, std::max(low, val));
}

template< typename T >
T lerp(const T from, const T to, const double amount) {
    return from + (to - from) * amount;
}

#define RUN_ONCE(body) { static bool _run_once_first = true; \
    if (_run_once_first) { _run_once_first = false; body; } }

#define _MACRO_COMBINER(X,Y) X##Y
#define MACRO_COMBINER(X,Y) _MACRO_COMBINER(X,Y)
#define MC(X,Y) MACRO_COMBINER(X,Y)
#define RUN_STATIC_NAMED(name,body) namespace { static struct MC(struct,name) { MC(struct,name) (){ body; } } MC(inst,name) ; }
#define RUN_STATIC(body) RUN_STATIC_NAMED(MC(StaticRunner,__LINE__), body)

#define rassert(expr, ...) \
    _rassert(static_cast< bool >(expr), #expr, __FILE__, __FUNCTION__, __LINE__, \
            # __VA_ARGS__, '\x00', ## __VA_ARGS__);
template< typename... Args >
void _rassert(bool pass, const char *expr, const char *file, const char *func, size_t line,
             Args &&...args) {
    if (pass) { return; }
    std::cerr << "At: " << file << " : " << func << " : " << line << '\n';
    std::cerr << "Assert failed: " << expr << std::endl;
    std::stringstream info;
    ((info << args << ", "), ...);
    const std::string infos = info.str();
    const auto split = infos.find('\x00');
    const std::string head = infos.substr(0, split - 2);
    const std::string tail = infos.substr(split + 3, infos.size() - split - 5);
    if (!head.empty()) {
        std::cerr << "Info:\n" << head << '\n' << tail << std::endl;
    }
    
    if (false) {
        // Approximately safely triggers an invalid read
        // so valgrind will print a call stack
        uint8_t * byte = static_cast< uint8_t * >(malloc(1));
        *byte = 4;
        free(byte);
        volatile uint8_t dummy = *byte;
        dummy *= 2;
    }

    abort();
}
