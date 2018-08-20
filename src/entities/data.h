#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <typeindex>

#include "signature.h"

namespace Entity {

class BaseData {
    public:
        virtual ~BaseData() { }
        virtual void add(uint64_t id) = 0;
        virtual void reserve(size_t more) = 0;
        virtual TypeID type() const = 0;
};
std::ostream &operator<<(std::ostream &os, const BaseData &bd);

#define DeclareDataType(T) \
    typedef Entity::Data< T > T ## Data; \

template< typename T >
class Data: public BaseData {
    public:
        typedef T Type;
        TypeID type() const override { return DataTypeID< T >(); }
        std::map< uint64_t, size_t > idToLow;
        std::vector< T > data;
        virtual ~Data() { }
        void add(uint64_t id) override {
            idToLow[id] = data.size();
            data.resize(data.size() + 1);
        }
        void reserve(size_t more) {
            data.reserve(data.size() + more);
        }
};

}
