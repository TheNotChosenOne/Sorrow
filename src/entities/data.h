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
        virtual void add(const uint64_t id) = 0;
        virtual void reserve(const size_t more) = 0;
        virtual void remove(const uint64_t id) = 0;
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
        std::map< uint64_t, size_t > lowToid;
        std::vector< T > data;
        virtual ~Data() { }
        void add(const uint64_t id) override {
            idToLow[id] = data.size();
            lowToid[data.size()] = id;
            data.resize(data.size() + 1);
        }
        void reserve(const size_t more) {
            data.reserve(data.size() + more);
        }
        void remove(const uint64_t id) override {
            const size_t back = data.size() - 1;
            const size_t backID = lowToid[back];
            const size_t eraseAt = idToLow[id];
            if (id == backID) {
                data.resize(data.size() - 1);
            } else {
                data[eraseAt] = std::move(data[back]);
                idToLow[backID] = eraseAt;
                lowToid[eraseAt] = backID;
            }
            lowToid.erase(back);
            idToLow.erase(id);
        }
};

}
