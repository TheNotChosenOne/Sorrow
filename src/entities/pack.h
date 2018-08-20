#pragma once

#include "utility/templates.h"

#include <tuple>
#include <vector>

namespace Entity {

template< size_t Index, typename Search, typename First, typename ...Rest >
struct PackAccess {
    static_assert(ArgCount< Rest... >::value > 0, "Couldn't find type");
    typedef typename PackAccess< Index + 1, Search, Rest... >::type type;
    static constexpr size_t index = Index;
};
template< size_t Index, typename Search, typename ...Rest >
struct PackAccess< Index, Search, Search, Rest... > {
    typedef PackAccess type;
    static constexpr size_t index = Index;
};

template< typename ...Args >
struct Pack {
    std::tuple< Args... > data;
    template< typename T >
    T &get() {
        constexpr size_t i = PackAccess< 0, T, Args... >::type::index;
        return std::get< i >(data);
    }
    template< typename T >
    const T &get() const {
        constexpr size_t i = PackAccess< 0, T, Args... >::type::index;
        return std::get< i >(data);
    }
};

template< typename ...Args >
struct Packs {
    std::tuple< ConstLifted< std::vector, Args > ... > data;
    using Mutable = std::tuple< std::vector< std::remove_const_t< Args > > ... >;
    
    template< typename T >
    ConstLifted< std::vector, T > &get() {
        constexpr size_t i = PackAccess< 0, T, Args... >::type::index;
        return std::get< i >(data);
    }

    template< typename T >
    const ConstLifted< std::vector, T > &get() const {
        constexpr size_t i = PackAccess< 0, T, Args... >::type::index;
        return std::get< i >(data);
    }

    template< typename T >
    T &at(size_t i) {
        constexpr size_t ti = PackAccess< 0, T, Args... >::type::index;
        return std::get< ti >(data)[i];
    }

    template< typename T >
    const T &at(size_t i) const {
        constexpr size_t ti = PackAccess< 0, T, Args... >::type::index;
        return std::get< ti >(data)[i];
    }
};

}
