#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <typeindex>

#include "utility/utility.h"
#include "signature.h"

class Core;

namespace Entity {

class BaseData {
    public:
        virtual ~BaseData() { }
        virtual bool has(const uint64_t id) const = 0;
        virtual void add(const uint64_t id) = 0;
        virtual void reserve(const size_t more) = 0;
        virtual void remove(const uint64_t id) = 0;
        virtual void initComponent(Core &core, const uint64_t id) = 0;
        virtual void deleteComponent(Core &core, const uint64_t id) = 0;
        virtual TypeID type() const = 0;
};
std::ostream &operator<<(std::ostream &os, const BaseData &bd);

#define DeclareDataType(T) \
    typedef Entity::Data< T > T ## Data; \

template< typename T >
void initComponent(Core &, uint64_t, T &) { }

template< typename T >
void deleteComponent(Core &, uint64_t, T &) { }

template< typename T >
class Data: public BaseData {
    public:
        typedef T Type;
        TypeID type() const override { return DataTypeID< T >(); }
        std::map< uint64_t, size_t > idToLow; // Converts from id to data index
        std::map< size_t, uint64_t > lowToid; // Converts from data index to id
        std::vector< T > data;
        virtual ~Data() { }

        bool has(const uint64_t id) const {
            return idToLow.find(id) != idToLow.end();
        }

        T &forID(uint64_t id) {
            const auto id_loc = idToLow.find(id);
            rassert(id_loc != idToLow.end(), "Entity does not have component", id, DataTypeName< T >());
            const size_t index = id_loc->second;
            rassert(index < data.size(), "Data chunk size is inconsistent", id, DataTypeName< T >());
            return data.at(index);
        }

        const T &forID(uint64_t id) const {
            const auto id_loc = idToLow.find(id);
            rassert(id_loc != idToLow.end(), "Entity does not have component", id, DataTypeName< T >());
            const size_t index = id_loc->second;
            rassert(index < data.size(), "Data chunk size is inconsistent", id, DataTypeName< T >());
            return data.at(index);
        }

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
        void initComponent(Core &core, const uint64_t id) override {
            T &t = data[idToLow[id]];
            Entity::initComponent< T >(core, id, t);
        }
        void deleteComponent(Core &core, const uint64_t id) override {
            T &t = data[idToLow[id]];
            Entity::deleteComponent< T >(core, id, t);
        }
};

}
