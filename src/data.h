#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <typeindex>

#include <ctti/type_id.hpp>

typedef ctti::type_id_t TypeID;
std::ostream &operator<<(std::ostream &os, const TypeID &tid);
constexpr bool operator<(const TypeID &left, const TypeID &right) {
    return left.hash() < right.hash();
}
template<>
struct std::less< TypeID > {
bool operator()(const TypeID &left, const TypeID &right) const {
    return ::operator<(left, right);
}
};

template< typename T >
constexpr TypeID DataTypeID() {
    return ctti::type_id< std::remove_const_t< T > >();
}

template< typename T >
constexpr ctti::detail::cstring DataTypeName() {
    return ctti::nameof< std::remove_const_t< T > >();
}

class BaseData {
    public:
        virtual ~BaseData() { }
        virtual void add(uint64_t id) = 0;
        virtual void reserve(size_t more) = 0;
        virtual TypeID type() const = 0;
};
std::ostream &operator<<(std::ostream &os, const BaseData &bd);

#define DeclareDataType(T) \
    typedef Data< T > T ## Data; \

template< typename T >
class Data: public BaseData {
    public:
        typedef T Type;
        TypeID type() const override { return DataTypeID< T >(); }
        std::map< uint64_t, size_t > idToLow;
        std::vector< T > data;
        virtual ~Data() { }
        void add(size_t id) override {
            idToLow[id] = data.size();
            data.resize(data.size() + 1);
        }
        void reserve(size_t more) {
            data.reserve(data.size() + more);
        }
};
