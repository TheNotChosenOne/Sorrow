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
template<>
struct std::hash< Signature > {
    std::size_t operator()(const Signature &sig) const {
        size_t h = 0;
        for (const Signature::value_type v : sig) {
            h ^= hash< Signature::value_type >()(v) + 0x9e3779b9 + (h  << 6) + (h >> 2);
        }
        return h;
    }
};
template<>
struct std::less< Signature > {
    bool operator()(const Signature &l, const Signature &r) const {
        if (l.size() != r.size()) { return l.size() < r.size(); }
        for (auto li = l.cbegin(), ri = r.cbegin();
             li != l.cend(); ++li, ++ri) {
            if (*li != *ri) { return ::operator<(*li, *ri); }
        }
        return false;
    }
};

template< typename... >
struct SetSignature;
template< typename T, typename ...Rest >
struct SetSignature< T, Rest... > {
    static void set(Signature &sig) {
        sig.insert(DataTypeID< T >());
        SetSignature< Rest... >::set(sig);
    }
    static void set(OrderedSignature &sig) {
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

template< typename T >
struct ConstLifter {
    typedef typename std::conditional< std::is_const< T >::value,
        const std::vector< std::remove_const_t< T > >,
        std::vector< std::remove_const_t< T > > >::type type;
};
template< size_t Index, typename Search, typename First, typename ...Rest >
struct CCAccess {
    typedef typename CCAccess< Index + 1, Search, Rest... >::type type;
    static constexpr size_t index = Index;
};

template< size_t Index, typename Search, typename ...Rest >
struct CCAccess< Index, Search, Search, Rest... > {
    typedef CCAccess type;
    static constexpr size_t index = Index;
};

template< typename ...Args >
struct ComponentCollection {
    std::tuple< typename ConstLifter< Args >::type ... > data;
    template< typename T >
    typename ConstLifter< T >::type &get() {
        constexpr size_t i = CCAccess< 0, T, Args... >::type::index;
        return std::get< i >(data);
    };
    template< typename T >
    const typename ConstLifter< T >::type &get() const {
        constexpr size_t i = CCAccess< 0, T, Args... >::type::index;
        return std::get< i >(data);
    };
    template< typename T >
    T &at(size_t i) {
        constexpr size_t ti = CCAccess< 0, T, Args... >::type::index;
        return std::get< ti >(data)[i];
    }
    template< typename T >
    const T &at(size_t i) const {
        constexpr size_t ti = CCAccess< 0, T, Args... >::type::index;
        return std::get< ti >(data)[i];
    }
};
template< typename ...Args >
struct CCTyper {
    typedef ComponentCollection< Args... > type;
};

template< typename BaseCC, typename ...Args >
struct PartitionExecHelper {
    typedef std::tuple< BaseCC, Args ... > Tuple;
    typedef std::function< void(BaseCC &, Args &...) > Func;
    typedef BaseCC Base;
};

template< size_t Index, typename... >
struct PEArgTyper;
template< size_t Index,
          typename ...Fs, template< typename ... > typename F,
          typename ...Rests >
struct PEArgTyper< Index, F< Fs... >, Rests... > {
    typedef F< Fs... > Arg;
    typedef PEArgTyper< Index + 1, Rests... > Rest;
    static constexpr size_t TupleIndex = Index;
};
template< size_t Index,
          typename ...Fs, template< typename ... > typename F >
struct PEArgTyper< Index, F< Fs... > > {
    typedef F< Fs... > Arg;
    typedef void Rest;
    static constexpr size_t TupleIndex = Index;
};

template< typename PEH, typename PEA >
struct PartitionExec {
    static void exec(Tracker &tracker, std::set< EntityID > &ids,
              typename PEH::Tuple &tuple,
              const typename PEH::Func &func);
};
template< typename PEH >
struct PartitionExec< PEH, void > {
    static void exec(Tracker &tracker, std::set< EntityID > &ids,
              typename PEH::Tuple &tuple,
              const typename PEH::Func &func);
};

template< typename Left, typename Right >
struct TypeEquals {
    static void check() {
        static_assert(std::is_same< Left, Right >::value, "Left is required, got Right");
    }
};

template< typename... >
struct TypeList;
template< typename F, typename... R >
struct TypeList< F, R... > {
    typedef F First;
    typedef TypeList< R... > Rest;
};
template< typename F >
struct TypeList< F > {
    typedef F First;
    typedef void Rest;
};
template<>
struct TypeList<> {
    typedef void First;
    typedef void Rest;
};

template< typename... >
struct is_prefix;
template< typename... Ls, template< typename... > typename L,
          typename... Rs, template< typename... > typename R >
struct is_prefix< L< Ls... >, R< Rs... > > {
    typedef typename L< Ls... >::First LF;
    typedef typename R< Rs... >::First RF;
    typedef typename L< Ls... >::Rest LR;
    typedef typename R< Rs... >::Rest RR;
    typedef typename std::conditional< std::is_same< LF, RF >::value,
            typename is_prefix< LR, RR >::type,
            std::false_type >::type type;
    static constexpr bool value = type::value;
};
template< typename... t, template< typename... > typename T >
struct is_prefix< T< t... >, void > {
    typedef std::false_type type;
    static constexpr bool value = type::value;
};
template< typename... t, template< typename... > typename T >
struct is_prefix< void, T< t... > > {
    typedef std::true_type type;
    static constexpr bool value = type::value;
};
template<>
struct is_prefix< void, void > {
    typedef std::true_type type;
    static constexpr bool value = type::value;
};

static bool typesSubset(const Signature &super, const Signature &sub) {
    if (sub.size() > super.size()) { return false; }
    for (const auto &s : sub) {
        if (!super.count(s)) { return false; }
    }
    return true;
}

template< typename ...Args >
struct Gatherer {
    std::set< EntityID > ids;
};
template< typename ...Args >
struct GathererTyper {
    typedef Gatherer< Args... > type;
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

        void findMatching(std::set< EntityID > &into, const Signature &sig) {
            for (const auto &pair : entities) {
                if (typesSubset(pair.first, sig)) {
                    std::copy(pair.second.begin(), pair.second.end(), std::inserter(into, into.begin()));
                }
            }
        }

        void restrictMatching(std::set< EntityID > &from, std::set< EntityID > &into, const Signature &sig) {
            for (const auto &pair : entities) {
                if (typesSubset(pair.first, sig)) {
                    for (const EntityID id : pair.second) {
                        const auto loc = from.find(id);
                        if (from.end() != loc) {
                            into.insert(from.extract(loc));
                        }
                    }
                }
            }
        }

        template< typename T >
        std::vector< T > getDataFor(const EntitySet &only) const {
            typedef Data< T > Source;
            typedef std::vector< T > Container;

            const TypeID tid = DataTypeID< T >();

            rassert(sources.count(tid), tid);

            Container into;
            const BaseData *base = sources.at(tid).get();
            rassert(DataTypeID< T >() == base->type(),
                    DataTypeName< T >(), base->type().name());
            const Source *sourcePtr = dynamic_cast< const Source * >(base);
            rassert(sourcePtr, tid, base->type().name());
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

            const TypeID tid = DataTypeID< T >();
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

        template< template< typename... > typename Container, typename ...Args >
        std::set< EntityID > getMatchingIDS(const Container< Args... > &) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());
            std::set< EntityID > ids;
            findMatching(ids, sig);
            return ids;
        }

        template< typename Base, typename... Args >
        void partitionExec(const typename PartitionExecHelper< Base, Args... >::Func &func) {
            typedef PartitionExecHelper< Base, Args... > PEH;
            typedef PEArgTyper< 1, Args... > Arg;

            typename PEH::Tuple tuple { };
            std::set< EntityID > ids = getMatchingIDS(std::get< 0 >(tuple));
            if (ids.empty()) { return; }

            PartitionExec< PartitionExecHelper< Base, Args... >, Arg >::exec(*this, ids, tuple, func);
        }

        template< typename... Args >
        void fancyExec(const std::function< void(typename GathererTyper< Args... >::type &gatherer) > &func) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());

            Gatherer< Args... > gatherer;
            findMatching(gatherer.ids, sig);
            if (gatherer.ids.empty()) { return; }

            func(gatherer);
        }

        template< typename... All, typename... Pre >
        void restrict(typename GathererTyper< Pre... >::type &gatherer,
                const std::function< void(typename CCTyper< All... >::type &) > &func) {
            static_assert(is_prefix< TypeList< Pre... >, TypeList< All... > >::type::value,
                    "Your restriction must have at least the types of the initial gather");
            const OrderedSignature osig = getOrderedSignature< All... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());

            std::set< EntityID > ids;
            restrictMatching(gatherer.ids, ids, sig);

            ComponentCollection< All... > cc;
            typedef std::tuple< std::vector< typename std::remove_const_t< All > > ... > Holder;
            populate< Holder, All... >(cc.data, ids);
            func(cc);
            emigrate< Holder, All... >(cc.data, ids);
        }

        template< typename... Args >
        void exec(const std::function< void(typename ConstLifter< Args >::type &...) > &func) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());

            std::set< EntityID > ids;
            findMatching(ids, sig);
            if (ids.empty()) { return; }

            typedef std::tuple< std::vector< typename std::remove_const_t< Args > > ... > Holder;
            Holder tupperware { };
            populate< Holder, Args... >(tupperware, ids);
            std::apply(func, tupperware);
            emigrate< Holder, Args... >(tupperware, ids);
        }

        template< typename T >
        std::vector< T > &getSource() {
            const TypeID tid = DataTypeID< T >();
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

template< typename... Args >
void popHelper(const Tracker &tracker, const std::set< EntityID > &only, std::tuple< Args... > &tuple) {
    Populater< std::tuple< Args... >, typename Args::value_type... >::populate(tracker, tuple, only, 0);
}

template< typename... Args >
void emHelper(Tracker &tracker, const std::set< EntityID > &only, std::tuple< Args... > &tuple) {
    Emigrater< std::tuple< Args... >, typename Args::value_type... >::emigrate(tracker, tuple, only, 0);
}

template< typename PEH, typename PEA >
void PartitionExec< PEH, PEA >::exec(Tracker &tracker, std::set< EntityID > &ids,
              typename PEH::Tuple &tuple,
              const typename PEH::Func &func) {
    auto &cc = std::get< PEA::TupleIndex >(tuple);
    auto &tofill = cc.data;

    std::set< EntityID > withThese = tracker.getMatchingIDS(cc);
    std::set< EntityID > only;
    std::set_intersection(withThese.begin(), withThese.end(),
                          ids.begin(), ids.end(), std::inserter(only, only.begin()));

    std::set< EntityID > diff;
    std::set_difference(ids.begin(), ids.end(), only.begin(), only.end(),
                        std::inserter(diff, diff.begin()));
    ids = std::move(diff);

    popHelper(tracker, only, tofill);
    PartitionExec< PEH, typename PEA::Rest >::exec(tracker, ids, tuple, func);
    emHelper(tracker, only, tofill);
}
template< typename PEH >
void PartitionExec< PEH, void >::exec(Tracker &tracker, std::set< EntityID > &ids,
              typename PEH::Tuple &tuple,
              const typename PEH::Func &func) {
    auto &tofill = std::get< 0 >(tuple).data;
    popHelper(tracker, ids, tofill);
    std::apply(func, tuple);
    emHelper(tracker, ids, tofill);
}
