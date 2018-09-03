#pragma once

#include <functional>

template< template< typename... > typename Container, typename T >
using ConstLifted = typename std::conditional< std::is_const< T >::value,
      const Container< std::remove_const_t< T > >,
      Container< std::remove_const_t< T > > >::type;

template< typename ...Args >
using ArgCount = std::integral_constant< size_t, sizeof...(Args) >;

template< typename ... >
struct FindIndex;

template< typename Find, typename First, typename ...Rest >
struct FindIndex< Find, First, Rest... > {
    static constexpr size_t value = FindIndex< Find, Rest... >::value + 1;
    static_assert(value <= sizeof...(Rest));
};
template< typename Find, typename ...Rest >
struct FindIndex< Find, Find, Rest... > {
    static constexpr size_t value = 0;
};
template< typename Find >
struct FindIndex< Find > {
    static constexpr size_t value = size_t(0);
};

template< typename ...Types >
struct FindIndices {
    template< typename T >
    static constexpr size_t index() { return FindIndex< T, Types... >::value; }
};

template< template< typename ... > typename Tup, typename ...Types >
struct Runtime {
    Tup< Types... > &t;
    Runtime(Tup< Types... > &tup): t(tup) { }

    template< typename T >
    T &get(size_t index) {
        constexpr size_t ind = FindIndex< T, Types... >::value;
        rassert(ind == index, "Index ", index, " should be ", ind);
        return std::get< ind >(t);
    }

    template< typename T >
    void set(size_t index, T &&t) {
        get< T >(index) = std::move(t);
    }
};

template< typename Left, typename Right >
struct TypeEquals {
    static void check() {
        static_assert(std::is_same< Left, Right >::value, "Left is required, got Right");
    }
};

template< typename ... >
struct ArgsExtract;
template< template< typename ... > typename Cont, typename ...Args >
struct ArgsExtract< Cont< Args... > > {
    template< template< typename ... > typename Wrap, template< typename ... > typename Transform >
    using Apply = Wrap< Transform< Args > ... >;
};
