#pragma once

#include <map>
#include <string>
#include <vector>
#include <typeindex>

typedef uint64_t TypeID;

class BaseData {
    public:
        virtual ~BaseData() { }
        virtual void add(uint64_t id) = 0;
        virtual TypeID type() const = 0;
        virtual const std::string &TypeName() const = 0;
};

template< typename... >
struct DataTypeName;
template< typename T >
struct DataTypeName< T > {
    static const std::string &name() {
        static const std::string &name = "Unnamed";
        return name;
    }
};
#define SetDataTypeName(T, n) \
    template<> struct DataTypeName< T > { static const std::string &name() {\
        static const std::string &name = n; return name; } };
#define DeclareDataType(T) \
    typedef Data< T > T ## Data; \
    SetDataTypeName(T, #T)

template< typename T >
class Data: public BaseData {
    public:
        typedef T Type;
        static void idFunc() { }
        TypeID type() const { return reinterpret_cast< TypeID >(idFunc); }
        std::map< uint64_t, size_t > idToLow;
        std::vector< T > data;
        virtual ~Data() { }
        void add(size_t id) override {
            idToLow[id] = data.size();
            data.resize(data.size() + 1);
        }
        const std::string &TypeName() const override {
            return DataTypeName< T >::name();
        }
};
