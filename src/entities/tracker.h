#pragma once

#include "entities/data.h"

#include <set>
#include <map>
#include <array>
#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <iostream>
#include <functional>
#include <shared_mutex>
#include <unordered_set>
#include <unordered_map>

#include "utility/utility.h"
#include "utility/templates.h"
#include "entities/pack.h"

struct Core;

namespace Entity {

typedef uint64_t EntityID;

class Tracker {
    public:
        typedef std::vector< EntityID > EntityVec;
        typedef std::unique_ptr< BaseData > SourcePtr;
        typedef std::map< TypeID, SourcePtr > Sources;
        typedef std::unordered_map< Signature, EntityVec > Entities;

        Sources sources;
        Entities entities;

        Sources nurserySources;
        Entities nursery;

    private:
        mutable std::shared_mutex tex;
        std::unordered_set< EntityID > doomed;
        EntityID nextID = 1;

        template< typename T >
        void addComponentForID(const EntityID id, const T &t, bool graduated) {
            (graduated ? getSource< std::remove_const_t< T > >() : getNurserySource< std::remove_const_t< T > >()).addThis(id, t);
        }

        template< typename T >
        void removeComponentForID(Core &core, const EntityID id) {
            auto &source = getSource< std::remove_const_t< T > >();
            source.deleteComponent(core, id);
            source.remove(id);
        }

        Signature getDuplicates(const OrderedSignature &sig) const;

    public:
        bool alive(const EntityID &eid) const;
        bool aliveWithLock(const EntityID &eid) const;
        bool zombie(const EntityID &eid) const;

        std::pair< Signature, bool > getSignature(const EntityID &eid) const;

        void withReadLock(const std::function< void() > &func);

        void withWriteLock(const std::function< void() > &func);

        Signature getRegisteredTypes() const;

        size_t sourceCount() const;

        template< typename T >
        SourcePtr &getSourcePtr() {
            const auto loc = sources.find(DataTypeID< std::remove_const_t< T > >());
            rassert(loc != sources.end(), "Data source is missing", DataTypeName< T >());
            return loc->second;
        }

        template< typename T >
        SourcePtr &getNurserySourcePtr() {
            const auto loc = nurserySources.find(DataTypeID< std::remove_const_t< T > >());
            rassert(loc != nurserySources.end(), "Data source is missing", DataTypeName< T >());
            return loc->second;
        }

        template< typename T >
        DataStorageType< T >::Type &getSource() { // TODO: Speedup ?
            const auto loc = sources.find(DataTypeID< std::remove_const_t< T > >());
            rassert(loc != sources.end(), "Data source is missing", DataTypeName< T >());
            return static_cast< DataStorageType< T >::Type & >(*loc->second);
        }

        template< typename T >
        DataStorageType< T >::Type &getNurserySource() {
            const auto loc = nurserySources.find(DataTypeID< std::remove_const_t< T > >());
            rassert(loc != nurserySources.end(), "Data source is missing", DataTypeName< T >());
            return static_cast< DataStorageType< T >::Type & >(*loc->second);
        }

        template< typename T >
        bool hasComponent(const EntityID &eid) {
            // Note: Does not support nursery entities
            std::shared_lock lock(tex);
            const auto typeID = DataTypeID< std::remove_const_t< T > >();
            for (const auto &pair : entities) {
                if (pair.first.count(typeID)) {
                    if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
                        return true;
                    }
                }
            }
            return false;
        }

        template< typename T >
        std::optional< std::reference_wrapper< T > > optComponent(const EntityID &eid) {
            std::shared_lock lock(tex);
            return getSource< std::remove_const_t< T > >().optForID(eid);
        }

        template< typename T >
        T &getComponent(const EntityID &eid) {
            std::shared_lock lock(tex);
            return getSource< std::remove_const_t< T > >().forID(eid);
        }

        template< typename T >
        const T &getComponent(const EntityID &eid) const {
            std::shared_lock lock(tex);
            return getSource< std::remove_const_t< T > >().forID(eid);
        }

        template< typename T >
        void addComponent(Core &core, const EntityID &eid, T &&component) {
            auto [sig, graduated] = getSignature(eid);
            std::unique_lock lock(tex);

            Sources *whichSources;
            Entities *whichEntities;
            if (graduated) {
                whichSources = &sources;
                whichEntities = &entities;
            } else {
                whichSources = &nurserySources;
                whichEntities = &nursery;
            }

            auto &group = whichEntities->at(sig);
            const auto pos = std::find(group.begin(), group.end(), eid);
            group.erase(pos);
            const auto res = sig.insert(DataTypeID< T >());
            rassert(res.second);
            (*whichEntities)[sig].push_back(eid);
            addComponentForID(eid, component, graduated);
            auto &source = *whichSources->at(DataTypeID< T >());
            source.initComponent(core, eid);
        }

        template< typename T >
        void removeComponent(Core &core, const EntityID &eid) {
            Signature sig = getSignature(eid).first; // Has its own lock
            std::unique_lock lock(tex);
            auto &group = entities.at(sig);
            const auto pos = std::find(group.begin(), group.end(), eid);
            group.erase(pos);
            const auto res = sig.erase(DataTypeID< T >());
            rassert(1 == res);
            entities[sig].push_back(eid);
            removeComponentForID< T >(core, eid);
        }

        EntityID createSigned(Core &core, const Signature &sig, size_t count=1);
        template< typename ...Args >
        EntityID create(Core &core, size_t count=1) {
            std::unique_lock lock(tex);
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            for (const auto tid : getDuplicates(osig)) {
                rassert(sources[tid]->isMulti(), "Duplicate single type in signature!")
            }
            return createSigned(core, sig, count);
        }

        template< typename ...Args >
        EntityID createWith(Core &core, const Args &... args) {
            std::unique_lock lock(tex);
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            for (const auto tid : getDuplicates(osig)) {
                rassert(sources.end() != sources.find(tid), "Missing type used!", osig);
                rassert(sources[tid]->isMulti(), "Duplicate single type in signature!")
            }

            const EntityID id = nextID++;
            nursery[sig].push_back(id);

            (addComponentForID(id, args, false), ...);
            for (const auto tid : sig) {
                nurserySources[tid]->initComponent(core, id);
            }

            return id;
        }

        template< typename T >
        void addSource() {
            std::unique_lock lock(tex);

            auto standard = std::make_unique< T >();

            if (sources.end() != sources.find(standard->type())) {
                return;
            }

            sources[standard->type()] = std::move(standard);

            auto younger = std::make_unique< T >();
            nurserySources[younger->type()] = std::move(younger);
        }

        void killEntity(Core &core, const EntityID id);
        void finalizeKills(Core &core);

        void killAll(Core &core);

        size_t count() const;

        void graduate();
};

}
