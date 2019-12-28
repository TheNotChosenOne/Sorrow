#pragma once

#include "utility/typelist.h"
#include "entities/tracker.h"
#include "entities/pack.h"
#include "utility/timers.h"

#include <utility>
#include <chrono>
#include <vector>
#include <mutex>

namespace Entity {

extern AccumulateTimer *k_entity_timer;
void addEntityTime(std::chrono::duration< double > seconds);

using IDMap = std::vector< EntityID >;

template< typename ...Packs >
using ExecFunc = std::function< void(std::pair< Packs, const IDMap > &...) >;

template< typename MainPack, typename ...Packss >
struct Exec {
    using Func = ExecFunc< MainPack, Packss... >;

    template< typename Type >
    static void getData(std::vector< Type > &v, const std::vector< EntityID > &ids, Tracker &tracker) {
        using BaseType = BaseFromSingle< Type >::Type;
        v.reserve(ids.size());

        const BaseData &baseSource = tracker.getSource< BaseType >();
        const typename DataStorageType< BaseType >::Type &source = static_cast< const DataStorageType< BaseType >::Type & >(baseSource);
        for (const auto id : ids) {
            source.addForID(id, v);
        }
    }

    template< typename ...Types >
    static void getIndices(const std::pair< Packs< Types... >, IDMap >&, Tracker &tracker, std::set< EntityID > &ids) {
        const Signature sig = getSignature< Types... >();
        for (auto &group : tracker.entities) {
            if (typesSubset(group.first, sig)) {
                for (const auto EID : group.second) {
                    ids.insert(EID);
                }
            }
        }
    }

    template< typename ...Types >
    static void populateMain(std::pair< Packs< Types... >, IDMap > &pair, Tracker &tracker, std::set< EntityID > &ids) {
        pair.second = decltype(pair.second)(ids.begin(), ids.end());
        using Muta = typename Packs< std::remove_const_t< Types > ... >::Mutable;
        using FI = FindIndices< Types... >;
        // This gets a non-const version of the data
        // I hope non-consts have the same alignments as consts
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstrict-aliasing"
        Muta &muta = reinterpret_cast< Muta & >(pair.first.data);
        #pragma GCC diagnostic pop
        (..., getData(std::get< FI::template index< Types >() >(muta), pair.second, tracker));
    }

    template< typename ...Types >
    static void populate(std::pair< Packs< Types... >, IDMap > &pair, Tracker &tracker, std::set< EntityID > &ids) {
        const Signature sig = getSignature< Types... >();
        for (auto &group : tracker.entities) {
            if (typesSubset(group.first, sig)) {
                for (const auto EID : group.second) {
                    const auto loc = ids.find(EID);
                    if (ids.end() != loc) {
                        ids.erase(loc);
                        pair.second.push_back(EID);
                    }
                }
            }
        }
        using Muta = typename Packs< std::remove_const_t< Types > ... >::Mutable;
        using FI = FindIndices< Types... >;
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstrict-aliasing"
        Muta &muta = reinterpret_cast< Muta & >(pair.first.data);
        #pragma GCC diagnostic pop
        (..., getData(std::get< FI::template index< Types >() >(muta), pair.second, tracker));
    }

    template< typename Type >
    static typename std::enable_if< std::is_const< Type >::value, void >::type
    setData(const DataStorageType< std::remove_const_t< Type > >::Container &, const std::vector< EntityID > &, Tracker &) {
        // Do nothing
    }

    template< typename Type >
    static typename std::enable_if< !std::is_const< Type >::value, void >::type
    setData(DataStorageType< std::remove_const_t< Type > >::Container &v, const std::vector< EntityID > &ids, Tracker &tracker) {
        BaseData &baseSource = tracker.getSource< Type >();
        typename DataStorageType< Type >::Type &source = static_cast< DataStorageType< Type >::Type & >(baseSource);
        for  (size_t i = 0; i < ids.size(); ++i) {
            source.setForID(ids[i], v[i]);
        }
    }

    template< typename ...Types >
    struct WriteExtractor {
        template< typename Type >
        static void wb(std::pair< Packs< Types... >, IDMap > &pair, Tracker &tracker) {
            setData< Type >(pair.first.template get< Type >(), pair.second, tracker);
        }
    };

    template< typename ...Types >
    static void writeBack(std::pair< Packs< Types... >, IDMap > &pair, Tracker &tracker) {
        using extract = WriteExtractor< Types... >;
        (..., extract::template wb< Types >(pair, tracker));
    }

    static void run(Tracker &tracker, const Func &f) {
        const auto start = std::chrono::high_resolution_clock::now();
        using Internal = std::tuple< std::pair< MainPack, IDMap >, std::pair< Packss, IDMap > ... >;
        using External = std::tuple< std::pair< MainPack, const IDMap >, std::pair< Packss, const IDMap > ... >;
        Internal data;
        using FI = FindIndices< MainPack, std::pair< Packss, IDMap > ... >;
        std::set< EntityID > ids;
        tracker.withReadLock([&]() {
            getIndices(std::get< 0 >(data), tracker, ids);
            (..., populate(std::get< FI::template index< std::pair< Packss, IDMap > >() >(data), tracker, ids));
            populateMain(std::get< 0 >(data), tracker, ids);
        });
        External &edata = reinterpret_cast< External & >(data);
        const auto funcStart = std::chrono::high_resolution_clock::now();
        std::apply(f, edata);
        const auto funcStop = std::chrono::high_resolution_clock::now();
        tracker.withWriteLock([&](){
            writeBack(std::get< 0 >(data), tracker);
            (..., writeBack(std::get< FI::template index< std::pair< Packss, IDMap > >() >(data), tracker));
        });
        const auto stop = std::chrono::high_resolution_clock::now();
        const auto duration = (stop - start) - (funcStop - funcStart);
        addEntityTime(duration);
    }
};

template< typename ...Types >
struct ExecSimple {
    using Func = std::function< void(const IDMap &, ConstLifted< std::vector, Types > &...) >;
    static void run(Tracker &tracker, const Func &f) {
        using Pack = Packs< Types... >;
        using Executor = Exec< Pack >;
        Executor::run(tracker, [&](std::pair< Pack, const IDMap > &pack) {
            std::apply(std::bind_front(f, pack.second), pack.first.data);
        });
    }
};

}
