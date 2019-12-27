#pragma once

#include <functional>

template< template< typename... > typename Container, typename T >
using ConstLifted = typename std::conditional< std::is_const< T >::value,
      const Container< std::remove_const_t< T > >,
      Container< std::remove_const_t< T > > >::type;

template< template< typename... > typename ContainerOuter, template< typename... > typename ContainerInner, typename T >
using ConstDoubleLifted = typename std::conditional< std::is_const< T >::value,
      const ContainerOuter< ContainerInner< std::remove_const_t< T > > >,
      ContainerOuter< ContainerInner< std::remove_const_t< T > > > >::type;

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
