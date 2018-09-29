#pragma once

#include "data.h"

#include <set>
#include <map>
#include <array>
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <functional>

#include "utility/utility.h"
#include "utility/templates.h"
#include "entities/pack.h"

class Core;

namespace Entity {

typedef uint64_t EntityID;

class Tracker {
    public:
        typedef std::vector< EntityID > EntityVec;
        typedef std::unique_ptr< BaseData > SourcePtr;

        std::map< TypeID, SourcePtr > sources;
        std::map< Signature, EntityVec > entities;

    private:
        EntityID nextID = 1;

        template< typename T >
        void addComponentForID(const EntityID id, const T &t) {
            auto &source = *sources.at(DataTypeID< T >());
            Data< T > &data = dynamic_cast< Data< T > & >(source);
            data.idToLow[id] = data.data.size();
            data.data.push_back(t);
        }

        template< typename T >
        void removeComponentForID(Core &core, const EntityID id) {
            auto &source = *sources.at(DataTypeID< T >());
            source.deleteComponent(core, id);
            source.remove(id);
        }

    public:
        bool alive(const EntityID &eid) const;

        Signature getSignature(const EntityID &eid) const;
        
        template< typename T >
        bool hasComponent(const EntityID &eid) {
            const auto typeID = DataTypeID< T >();
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
            const auto typeID = DataTypeID< T >();
            auto &source = static_cast< Data< T > & >(*sources.at(typeID));
            auto loc = source.idToLow.find(eid);
            if (source.idToLow.end() == loc) { return std::nullopt; }
            return std::optional< std::reference_wrapper< T > >{source.data[loc->second]};
        }

        template< typename T >
        T &getComponent(const EntityID &eid) {
            const auto typeID = DataTypeID< T >();
            auto &source = static_cast< Data< T > & >(*sources.at(typeID));
            return source.data[source.idToLow.at(eid)];
        }

        template< typename T >
        const T &getComponent(const EntityID &eid) const {
            const auto typeID = DataTypeID< T >();
            const auto &source = static_cast< Data< T > & >(*sources.at(typeID));
            return source.data[source.idToLow.at(eid)];
        }

        template< typename T >
        void addComponent(Core &core, const EntityID &eid, T &&component) {
            Signature sig = getSignature(eid);
            auto &group = entities.at(sig);
            const auto pos = std::find(group.begin(), group.end(), eid);
            group.erase(pos);
            const auto res = sig.insert(DataTypeID< T >());
            rassert(res.second);
            entities[sig].push_back(eid);
            addComponentForID(eid, component);
            auto &source = *sources.at(DataTypeID< T >());
            source.initComponent(core, eid);
        }

        template< typename T >
        void removeComponent(Core &core, const EntityID &eid) {
            Signature sig = getSignature(eid);
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
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());
            return createSigned(core, sig, count);
        }

        template< typename ...Args >
        EntityID createWith(Core &core, const Args &... args) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());

            const EntityID id = nextID++;
            entities[sig].push_back(id);

            (addComponentForID(id, args), ...);
            for (const auto tid : sig) {
                sources[tid]->initComponent(core, id);
            }


            return id;
        }

        void killEntity(Core &core, const EntityID id);

        void killAll(Core &core);

        void addSource(SourcePtr &&ptr);
};

}
