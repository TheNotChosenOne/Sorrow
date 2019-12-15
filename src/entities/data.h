#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <typeindex>
#include <functional>

#include "utility/utility.h"
#include "entities/signature.h"

struct Core;

namespace Entity {

class BaseData {
    public:
        virtual ~BaseData() { }
        virtual bool isMulti() const = 0;
        virtual size_t has(const uint64_t id) const = 0;
        virtual void add(const uint64_t id) = 0;
        virtual void reserve(const size_t more) = 0;
        virtual void remove(const uint64_t id) = 0;
        virtual void initComponent(Core &core, const uint64_t id) = 0;
        virtual void deleteComponent(Core &core, const uint64_t id) = 0;
        virtual void graduateFrom(BaseData &other) = 0;
        virtual TypeID type() const = 0;
};
std::ostream &operator<<(std::ostream &os, const BaseData &bd);

#define DeclareDataType(T) \
    typedef Entity::Data< T > T ## Data; \

#define DeclareMultiDataType(T) \
    typedef Entity::MultiData< T > T ## Data; \

template< typename T >
void initComponent(Core &, uint64_t, T &) { }

template< typename T >
void deleteComponent(Core &, uint64_t, T &) { }

template< typename T >
class Data: public BaseData {
    public:
        typedef T Type;
        bool isMulti() const { return false; }
        TypeID type() const override { return DataTypeID< T >(); }
        std::map< uint64_t, size_t > idToLow; // Converts from id to data index
        std::map< size_t, uint64_t > lowToid; // Converts from data index to id
        std::vector< T > data;
        virtual ~Data() { }

        void graduateFrom(BaseData &baseOther) override {
            rassert(type() == baseOther.type(), "Cannot graduate from a different type!", type(), baseOther.type());
            Data< T > &other = static_cast< Data< T > & >(baseOther);

            data.reserve(data.size() + other.data.size());
            for (size_t i = 0; i < other.data.size(); ++i) {
                const uint64_t id = other.lowToid[i];
                idToLow[id] = data.size();
                lowToid[data.size()] = id;
                data.emplace_back(other.data[i]);
            }

            other.idToLow.clear();
            other.lowToid.clear();
            other.data.clear();
        }

        size_t has(const uint64_t id) const override {
            return idToLow.find(id) != idToLow.end() ? 1 : 0;
        }

        T &forID(const uint64_t id) {
            const auto id_loc = idToLow.find(id);
            rassert(id_loc != idToLow.end(), "Entity does not have component", id, DataTypeName< T >());
            const size_t index = id_loc->second;
            rassert(index < data.size(), "Data chunk size is inconsistent", id, DataTypeName< T >());
            return data.at(index);
        }

        const T &forID(const uint64_t id) const {
            const auto id_loc = idToLow.find(id);
            rassert(id_loc != idToLow.end(), "Entity does not have component", id, DataTypeName< T >());
            const size_t index = id_loc->second;
            rassert(index < data.size(), "Data chunk size is inconsistent", id, DataTypeName< T >());
            return data.at(index);
        }

        std::optional< std::reference_wrapper< T > > optForID(const uint64_t id) {
            const auto id_loc = idToLow.find(id);
            if (idToLow.end() == id_loc) {
                return std::nullopt;
            }
            const size_t index = id_loc->second;
            rassert(index < data.size(), "Data chunk size is inconsistent", id, DataTypeName< T >());
            return std::optional< std::reference_wrapper< T > >{data.at(index)};
        }

        std::optional< std::reference_wrapper< const T > > optForID(const uint64_t id) const {
            const auto id_loc = idToLow.find(id);
            if (idToLow.end() == id_loc) {
                return std::nullopt;
            }
            const size_t index = id_loc->second;
            rassert(index < data.size(), "Data chunk size is inconsistent", id, DataTypeName< T >());
            return std::optional< std::reference_wrapper< const T > >{data.at(index)};
        }

        void reserve(const size_t more) override {
            data.reserve(data.size() + more);
        }

        void add(const uint64_t id) override {
            idToLow[id] = data.size();
            lowToid[data.size()] = id;
            data.resize(data.size() + 1);
        }

        void addThis(const uint64_t id, const T &t) {
            idToLow[id] = data.size();
            lowToid[data.size()] = id;
            data.emplace_back(t);
        }

        void remove(const uint64_t id) override {
            const size_t back = data.size() - 1;
            const size_t backID = lowToid[back];
            const size_t eraseAt = idToLow[id];
            if (id != backID) {
                data[eraseAt] = std::move(data[back]);
                idToLow[backID] = eraseAt;
                lowToid[eraseAt] = backID;
            }
            data.resize(data.size() - 1);
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

template< typename T >
class MultiData: public BaseData {
    public:
        typedef T Type;
        bool isMulti() const { return true; }
        TypeID type() const override { return DataTypeID< T >(); }
        std::map< uint64_t, std::vector< size_t > > idToLow; // Converts from id to data index
        std::map< size_t, uint64_t > lowToid; // Converts from data index to id
        std::vector< T > data;
        virtual ~MultiData() { }

        void graduateFrom(BaseData &baseOther) override {
            rassert(type() == baseOther.type(), "Cannot graduate from a different type!", type(), baseOther.type());
            MultiData< T > &other = static_cast< MultiData< T > & >(baseOther);

            data.reserve(data.size() + other.data.size());
            for (size_t i = 0; i < other.data.size(); ++i) {
                const uint64_t id = other.lowToid[i];
                idToLow[id].push_back(data.size());
                lowToid[data.size()] = id;
                data.emplace_back(other.data[i]);
            }

            other.idToLow.clear();
            other.lowToid.clear();
            other.data.clear();
        }

        size_t has(const uint64_t id) const override {
            const auto loc = idToLow.find(id);
            if (idToLow.end() == loc) { return 0; }
            return loc->second.size();
        }

        std::vector< std::reference_wrapper< T > > forID(const uint64_t id) {
            std::vector< std::reference_wrapper< T > > refs;
            for (const size_t index : idToLow.at(id)) {
                refs.push_back(data[index]);
            }
            return refs;
        }

        const T &forID(const uint64_t id) const {
            std::vector< std::reference_wrapper< const T > > refs;
            for (const size_t index : idToLow.at(id)) {
                refs.push_back(data[index]);
            }
            return refs;
        }

        std::vector< std::reference_wrapper< T > > optForID(const uint64_t id) {
            std::vector< std::reference_wrapper< T > > refs;
            const auto loc = idToLow.find(id);
            if (idToLow.end() == loc) { return refs; }
            for (const size_t index : *loc) {
                refs.push_back(data[index]);
            }
            return refs;
        }

        std::vector< std::reference_wrapper< const T > > optForID(const uint64_t id) const {
            std::vector< std::reference_wrapper< const T > > refs;
            const auto loc = idToLow.find(id);
            if (idToLow.end() == loc) { return refs; }
            for (const size_t index : *loc) {
                refs.push_back(data[index]);
            }
            return refs;
        }

        void reserve(const size_t more) override {
            data.reserve(data.size() + more);
        }

        void add(const uint64_t id) override {
            idToLow[id].push_back(data.size());
            lowToid[data.size()] = id;
            data.resize(data.size() + 1);
        }

        void addThis(const uint64_t id, const T &t) {
            idToLow[id].push_back(data.size());
            lowToid[data.size()] = id;
            data.emplace_back(t);
        }

        void remove(const uint64_t id) override {
            // Warning: This removes all of them for this id
            // TODO: This could probably be more efficient
            size_t back = data.size() - 1;
            for (const size_t erase_at : idToLow[id]) {
                if (erase_at == back) {
                    lowToid.erase(back);
                    // idToLow will be deleted in bulk later
                    --back;
                    continue;
                }

                data[erase_at] = std::move(data[back]);
                const auto backID = lowToid[back];
                lowToid[erase_at] = backID;
                auto &siblings = idToLow[backID];
                auto loc = std::find(siblings.begin(), siblings.end(), back);
                *loc = erase_at;
                --back;
            }
            data.resize(back + 1);
            idToLow.erase(id);
        }

        void initComponent(Core &core, const uint64_t id) override {
            for (const size_t index : idToLow[id]) {
                Entity::initComponent< T >(core, id, data[index]);
            }
        }

        void deleteComponent(Core &core, const uint64_t id) override {
            for (const size_t index : idToLow[id]) {
                Entity::deleteComponent< T >(core, id, data[index]);
            }
        }
};

}
