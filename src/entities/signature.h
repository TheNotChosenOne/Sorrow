#pragma once

#include <ctti/type_id.hpp>
#include <string_view>
#include <vector>
#include <set>

namespace Entity {

typedef ctti::type_id_t TypeID;

}

constexpr bool operator<(const ctti::type_id_t &left, const ctti::type_id_t &right) {
    const auto &l = left.name();
    const auto &r = right.name();
    return std::string_view(l.begin(), l.size()) < std::string_view(r.begin(), r.size());
}


namespace Entity {

typedef std::set< TypeID > Signature;
typedef std::vector< TypeID > OrderedSignature;

}

std::ostream &operator<<(std::ostream &os, const Entity::TypeID &tid);
std::ostream &operator<<(std::ostream &os, const Entity::Signature &sig);
std::ostream &operator<<(std::ostream &os, const Entity::OrderedSignature &sig);

template<>
struct std::less< Entity::TypeID > {
bool operator()(const Entity::TypeID &left, const Entity::TypeID &right) const {
    return ::operator<(left, right);
}
};

template<>
struct std::hash< Entity::Signature > {
    std::size_t operator()(const Entity::Signature &sig) const {
        size_t h = 0;
        for (const Entity::Signature::value_type &v : sig) {
            h ^= hash< Entity::Signature::value_type >()(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

template<>
struct std::less< Entity::Signature > {
    bool operator()(const Entity::Signature &l, const Entity::Signature &r) const {
        if (l.size() != r.size()) { return l.size() < r.size(); }
        for (auto li = l.cbegin(), ri = r.cbegin();
             li != l.cend(); ++li, ++ri) {
            if (*li != *ri) {
                return ::operator<(*li, *ri);
            }
        }
        return false;
    }
};

namespace Entity {

template< typename T >
constexpr TypeID DataTypeID() {
    return ctti::type_id< std::remove_const_t< T > >();
}

template< typename T >
constexpr ctti::detail::cstring DataTypeName() {
    return ctti::nameof< std::remove_const_t< T > >();
}

template< typename... >
struct SetSignature;
template< typename T, typename ...Rest >
struct SetSignature< T, Rest... > {
    static inline void set(Signature &sig) {
        sig.insert(DataTypeID< T >());
        SetSignature< Rest... >::set(sig);
    }
    static inline void set(OrderedSignature &sig) {
        sig.push_back(DataTypeID< T >());
        SetSignature< Rest... >::set(sig);
    }
};

template<>
struct SetSignature<> {
    static void set(Signature &) { }
    static void set(OrderedSignature &) { }
};

template< typename ...Args >
Signature getSignature() {
    Signature sig;
    SetSignature< Args... >::set(sig);
    return sig;
}

template< typename ...Args >
OrderedSignature getOrderedSignature() {
    OrderedSignature sig;
    SetSignature< Args... >::set(sig);
    return sig;
}

bool typesSubset(const Signature &super, const Signature &sub);

std::string signatureString(const Signature &sig);
std::string signatureString(const OrderedSignature &sig);

}
