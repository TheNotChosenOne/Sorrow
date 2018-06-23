#pragma once

#include "data.h"

#include <set>
#include <map>
#include <array>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "utility.h"

typedef uint64_t EntityID;

typedef std::set< TypeID > Signature;
namespace std {
template<>
struct hash< Signature > {
    std::size_t operator()(const Signature &sig) const {
        size_t h = 0;
        for (const Signature::value_type v : sig) {
            h ^= hash< Signature::value_type >()(v) + 0x9e3779b9 + (h  << 6) + (h >> 2);
        }
        return h;
    }
};
}

template< typename... >
struct DataTypeID;
template< typename T >
struct DataTypeID< T > {
    static TypeID id() {
        static Data< T > instance;
        return instance.type();
    }
};

template< typename... >
struct SetSignature;
template< typename T, typename ...Rest >
struct SetSignature< T, Rest... > {
    static void set(Signature &sig) {
        sig.insert(DataTypeID< T >::id());
        SetSignature< Rest... >::set(sig);
    }
};
template<>
struct SetSignature<> {
    static void set(Signature &) { }
};

template< typename ...Args >
void setSignature(Signature &sig) {
    SetSignature< Args... >::set(sig);
}

template< typename ...Args >
Signature getSignature() {
    Signature sig;
    SetSignature< Args... >::set(sig);
    return sig;
}

class Tracker;

template< typename TupleType, typename... >
struct Populater;
template< typename TupleType, typename T, typename ...Rest >
struct Populater< TupleType, T, Rest... > {
    static void populate(const Tracker &t, TupleType &tup,
                         const std::set< EntityID > &only, const Signature &sig, size_t index);
};
template< typename TupleType >
struct Populater< TupleType > {
    static void populate(const Tracker &, TupleType &,
                         const std::set< EntityID > &, const Signature &sig, size_t index) {
        rassert(index == sig.size());
    }
};

template< typename TupleType, typename... >
struct Emigrater;
template< typename TupleType, typename T, typename ...Rest >
struct Emigrater< TupleType, T, Rest... > {
    static void emigrate(Tracker &t, TupleType &tup,
                         const std::set< EntityID > &only, const Signature &sig,
                         const std::vector< bool > &write, size_t index);
};
template< typename TupleType >
struct Emigrater< TupleType > {
    static void emigrate(Tracker &, TupleType &,
                         const std::set< EntityID > &, const Signature &sig,
                         const std::vector< bool > &, size_t index) {
        rassert(index == sig.size());
    }
};

class Tracker {
    public:
        typedef std::vector< EntityID > EntityVec;
        typedef std::set< EntityID > EntitySet;

    private:
        typedef std::unique_ptr< BaseData > SourcePtr;

        EntityID nextID = 0;

    public:
        std::map< TypeID, SourcePtr > sources;
        std::map< Signature, EntityVec > entities;

        EntityID create(const Signature &sig);

        void findMatching(std::vector< EntityID > &into, const Signature &sig) {
            const auto loc = entities.find(sig);
            if (entities.end() != loc) { into = loc->second; }
        }

        void findMatching(std::set< EntityID > &into, const Signature &sig) {
            const auto loc = entities.find(sig);
            if (entities.end() != loc) {
                into = std::set< EntityID >(loc->second.begin(), loc->second.end());
            }
        }

        template< typename T >
        std::unique_ptr< std::vector< T > > getDataFor(const EntitySet &only, TypeID tid) const {
            typedef Data< T > Source;
            typedef std::vector< T > Container;
            typedef std::unique_ptr< Container > ContainerPtr;

            rassert(sources.count(tid), tid);

            ContainerPtr ptr = std::make_unique< Container >();
            Container &into = *ptr;
            const Source &data = dynamic_cast< const Source & >(*sources.at(tid));
            for (const EntityID id : only) {
                const size_t low = data.idToLow.at(id);
                into.push_back(data.data[low]);
            }

            return ptr;
        }

        template< typename T >
        void setDataFor(const EntitySet &only, TypeID tid, const std::vector< T > &data) {
            typedef Data< T > Source;

            rassert(sources.count(tid), tid);

            Source &dataSource = dynamic_cast< Source & >(*sources.at(tid));
            std::vector< T > &source = dataSource.data;
            size_t index = 0;
            for (const EntityID id : only) {
                const size_t low = dataSource.idToLow.at(id);
                source[low] = std::move(data[index++]);
            }
        }

        template< typename TupleType, typename... Args >
        void populate(TupleType &tup, const EntitySet &only, const Signature &sig) const {
            rassert(std::tuple_size< TupleType >::value == sig.size(), sig);
            Populater< TupleType, Args... >::populate(*this, tup, only, sig, 0);
        }

        template< typename TupleType, typename... Args >
        void emigrate(TupleType &tup, const EntitySet &only, const Signature &sig,
                      const std::vector< bool > &write) {
            rassert(std::tuple_size< TupleType >::value == sig.size(), sig);
            rassert(std::tuple_size< TupleType >::value == write.size(), sig);
            Emigrater< TupleType, Args... >::emigrate(*this, tup, only, sig, write, 0);
        }

        template< typename TupleType, typename FuncType, typename... Args >
        void executeOn(const FuncType &func, const std::vector< bool > &write) {
            Signature sig = getSignature< Args... >();
            rassert(sig.size() == write.size(), sig, write);
            TupleType tupperware;
            std::set< EntityID > ids;
            findMatching(ids, sig);
            if (ids.empty()) { return; }
            populate< TupleType, Args... >(tupperware, ids, sig);
            func(tupperware);
            emigrate< TupleType, Args... >(tupperware, ids, sig, write);
        }

        template< typename T >
        const std::vector< T > &getSource() {
            const TypeID tid = DataTypeID< T >::id();
            rassert(sources.count(tid), tid);
            return dynamic_cast< Data< T > & >(*sources.at(tid)).data;
        }
};

namespace {

template< typename Tuple, typename V >
static inline void runtimeTupleSet(Tuple &t, size_t index, V &&v) {
    const size_t offset = (std::tuple_size< Tuple >::value - index) - 1;
    void **hack = reinterpret_cast< void ** >(&t) + offset;
    *hack = nullptr;
    V *ptr = reinterpret_cast< V * >(hack);
    *ptr = std::move(v);
}

template< typename Tuple, typename V >
static inline V &runtimeTupleGet(Tuple &t, size_t index) {
    const size_t offset = (std::tuple_size< Tuple >::value - index) - 1;
    void **hack = reinterpret_cast< void ** >(&t) + offset;
    V *ptr = reinterpret_cast< V * >(hack);
    return *ptr;
}

}

template< typename TupleType, typename T, typename ...Rest >
void Populater< TupleType, T, Rest... >::populate(
        const Tracker &t, TupleType &tup, const std::set< EntityID > &only,
        const Signature &sig, size_t index) {
    auto it = sig.begin();
    std::advance(it, index);
    runtimeTupleSet(tup, index, t.getDataFor< T >(only, *it));
    Populater< TupleType, Rest... >::populate(t, tup, only, sig, index + 1);
}

template< typename TupleType, typename T, typename ...Rest >
void Emigrater< TupleType, T, Rest... >::emigrate(Tracker &t, TupleType &tup,
                         const std::set< EntityID > &only, const Signature &sig,
                         const std::vector< bool > &write, size_t index) {
    typedef std::vector< T > Container;
    typedef std::unique_ptr< Container > ContainerPtr;
    auto it = sig.begin();
    std::advance(it, index);
    t.setDataFor< T >(only, *it, *runtimeTupleGet< TupleType, ContainerPtr >(tup, index));
    Emigrater< TupleType, Rest... >::emigrate(t, tup, only, sig, write, index + 1);
}

std::string sigToStr(const Signature &sig, const Tracker &track);
