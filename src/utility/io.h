#pragma once

#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <memory>
#include <set>
#include <map>

template< typename T >
std::string str(const T &x) {
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template< typename T >
std::ostream &operator<<(std::ostream &os, const std::vector< T > &v) {
    os << '(' << v.size() << ")[ ";
    for (const auto &x : v) {
        os << x << ' ';
    }
    os << ']';
    return os;
}

template< typename K, typename V >
std::ostream &operator<<(std::ostream &os, const std::pair< K, V > &p) {
    return (os << '(' << p.first << ", " << p.second << ')');
}

template< template< typename ... > typename C, typename T >
std::ostream &dumpContainer(std::ostream &os, const C< T > &s) {
    os <<'(' << s.size() << ")[ ";
    for (const auto &v : s) {
        os << v << ' ';
    }
    return (os << '}');
}

template< typename T >
std::ostream &operator<<(std::ostream &os, const std::set< T > &s) {
    return dumpContainer(os, s);
}

template< typename T >
std::ostream &operator<<(std::ostream &os, const std::unordered_set< T > &s) {
    return dumpContainer(os, s);
}

template< typename K, typename V >
std::ostream &operator<<(std::ostream &os, const std::map< K, V > &s) {
    return dumpContainer(os, s);
}

template< typename K, typename V >
std::ostream &operator<<(std::ostream &os, const std::unordered_map< K, V > &s) {
    return dumpContainer(os, s);
}

template< typename T >
std::ostream &operator<<(std::ostream &os, const std::unique_ptr< T > &ptr) {
    os << "unique {";
    if (ptr) { os << *ptr; }
    return (os << ')');
}

template< typename T >
std::ostream &operator<<(std::ostream &os, const std::shared_ptr< T > &ptr) {
    os << "shared {";
    if (ptr) { os << *ptr; }
    return (os << ')');
}
