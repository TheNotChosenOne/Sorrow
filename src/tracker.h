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
typedef std::vector< TypeID > OrderedSignature;
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
        static Data< std::remove_const_t< T > > instance;
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
    static void set(OrderedSignature &sig) {
        sig.push_back(DataTypeID< T >::id());
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
class Tracker;

std::string sigToStr(const Signature &sig, const Tracker &track);

template< typename TupleType, typename... >
struct Populater;
template< typename TupleType, typename T, typename ...Rest >
struct Populater< TupleType, T, Rest... > {
    static void populate(const Tracker &t, TupleType &tup,
                         const std::set< EntityID > &only, size_t index);
};
template< typename TupleType >
struct Populater< TupleType > {
    static void populate(const Tracker &, TupleType &,
                         const std::set< EntityID > &, size_t) {
    }
};

template< typename TupleType, typename... >
struct Emigrater;
template< typename TupleType, typename T, typename ...Rest >
struct Emigrater< TupleType, T, Rest... > {
    static void emigrate(Tracker &t, TupleType &tup,
                         const std::set< EntityID > &only, size_t index);
};
template< typename TupleType >
struct Emigrater< TupleType > {
    static void emigrate(Tracker &, TupleType &,
                         const std::set< EntityID > &, size_t) {
    }
};

template< typename Left, typename Right >
struct TypeEquals {
    static void check() {
        static_assert(std::is_same< Left, Right >::value, "Left is required, got Right");
    }
};

static bool typesSubset(const Signature &super, const Signature &sub) {
    if (sub.size() > super.size()) { return false; }
    for (const auto &s : sub) {
        if (!super.count(s)) { return false; }
    }
    return true;
}

template< typename T >
struct ConstLifter {
    typedef typename std::conditional< std::is_const< T >::value,
        const std::vector< std::remove_const_t< T > >,
        std::vector< std::remove_const_t< T > > >::type type;
};

class Tracker {
    public:
        typedef std::vector< EntityID > EntityVec;
        typedef std::set< EntityID > EntitySet;

    private:
        typedef std::unique_ptr< BaseData > SourcePtr;

        EntityID nextID = 1;

    public:
        std::map< TypeID, SourcePtr > sources;
        std::map< Signature, EntityVec > entities;

        EntityID create(const Signature &sig, size_t count=1);

        void addSource(SourcePtr &&ptr);

        void findMatching(std::vector< EntityID > &into, const Signature &sig) {
            for (const auto &pair : entities) {
                if (typesSubset(pair.first, sig)) {
                    std::copy(pair.second.begin(), pair.second.end(), std::back_inserter(into));
                }
            }
        }

        void findMatching(std::set< EntityID > &into, const Signature &sig) {
            for (const auto &pair : entities) {
                if (typesSubset(pair.first, sig)) {
                    std::copy(pair.second.begin(), pair.second.end(), std::inserter(into, into.begin()));
                }
            }
        }

        template< typename T >
        std::vector< T > getDataFor(const EntitySet &only) const {
            typedef Data< T > Source;
            typedef std::vector< T > Container;

            const TypeID tid = DataTypeID< T >::id();

            rassert(sources.count(tid), tid);

            Container into;
            const BaseData *base = sources.at(tid).get();
            rassert(DataTypeName< T >::name() == base->TypeName(),
                    DataTypeName< T >::name(), base->TypeName());
            const Source *sourcePtr = dynamic_cast< const Source * >(base);
            rassert(sourcePtr, tid, base->type(), base->TypeName());
            const Source &data = *sourcePtr;
            for (const EntityID id : only) {
                const size_t low = data.idToLow.at(id);
                into.push_back(data.data[low]);
            }

            return into;
        }

        template< typename T >
        void setDataFor(const EntitySet &only, const std::vector< T > &data) {
            typedef Data< T > Source;

            const TypeID tid = DataTypeID< T >::id();
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
        void populate(TupleType &tup, const EntitySet &only) const {
            Populater< TupleType, Args... >::populate(*this, tup, only, 0);
        }

        template< typename TupleType, typename... Args >
        void emigrate(TupleType &tup, const EntitySet &only) {
            Emigrater< TupleType, Args... >::emigrate(*this, tup, only, 0);
        }

        template< typename... Args >
        void exec(const std::function< void(typename ConstLifter< Args >::type &...) > &func) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());

            typedef std::tuple< std::vector< typename std::remove_const_t< Args > > ... > Holder;
            Holder tupperware { };
            std::set< EntityID > ids;
            findMatching(ids, sig);
            if (ids.empty()) { return; }
            populate< Holder, Args... >(tupperware, ids);
            std::apply(func, tupperware);
            emigrate< Holder, Args... >(tupperware, ids);
        }

        template< typename T >
        std::vector< T > &getSource() {
            const TypeID tid = DataTypeID< T >::id();
            rassert(sources.count(tid), tid);
            return dynamic_cast< Data< T > & >(*sources.at(tid)).data;
        }
};

namespace {

template< typename Tuple, typename V >
static inline V &runtimeTupleGet(Tuple &t, size_t index) {
    const size_t per = sizeof(std::get< 0 >(t));
    const size_t offset = (std::tuple_size< Tuple >::value - index) - 1;
    char *hack = reinterpret_cast< char * >(&t) + offset * per;
    V *ptr = reinterpret_cast< V * >(hack);
    return *ptr;
}

template< typename Tuple, typename V >
static inline void runtimeTupleSet(Tuple &t, size_t index, V &&v) {
    runtimeTupleGet< Tuple, V >(t, index) = std::move(v);
}

}

template< typename TupleType, typename T, typename ...Rest >
void Populater< TupleType, T, Rest... >::populate(
        const Tracker &t, TupleType &tup, const std::set< EntityID > &only, size_t index) {
    auto p = t.getDataFor< std::remove_const_t< T > >(only);
    rassert(p.size() == only.size(), p.size(), only.size());
    runtimeTupleSet(tup, index, std::move(p));
    Populater< TupleType, Rest... >::populate(t, tup, only, index + 1);
}

template< typename TupleType, typename T, typename ...Rest >
void Emigrater< TupleType, T, Rest... >::emigrate(Tracker &t, TupleType &tup,
                         const std::set< EntityID > &only, size_t index) {
    typedef typename ConstLifter< T >::type Container;
    if (!std::is_const< T >::value) {
        t.setDataFor< std::remove_const_t< T > >(only, runtimeTupleGet< TupleType, Container >(tup, index));
    }
    Emigrater< TupleType, Rest... >::emigrate(t, tup, only, index + 1);
}
