#pragma once

#include <utility>

template< typename ...Args >
struct TypeList {
    using MutableList = std::tuple< std::remove_const_t< Args > ... >;
    using ConstList = std::tuple< const Args ... >;
    using List = std::tuple< Args ... >;

    MutableList list;
};
