#pragma once

#include "data.h"

#include <set>
#include <map>
#include <array>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "utility/utility.h"
#include "utility/templates.h"
#include "entities/pack.h"

namespace Entity {

typedef uint64_t EntityID;

class Tracker {
    public:
        typedef std::vector< EntityID > EntityVec;
        typedef std::unique_ptr< BaseData > SourcePtr;

    private:
        EntityID nextID = 1;

    public:
        std::map< TypeID, SourcePtr > sources;
        std::map< Signature, EntityVec > entities;

        EntityID createSigned(const Signature &sig, size_t count=1);
        template< typename ...Args >
        EntityID create(size_t count=1) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());
            return createSigned(sig, count);
        }

        template< typename T >
        void addComponentForID(const EntityID id, const T &t) {
            auto &source = *sources.at(DataTypeID< T >());
            Data< T > &data = dynamic_cast< Data< T > & >(source);
            data.idToLow[id] = data.data.size();
            data.data.push_back(t);
        }

        template< typename ...Args >
        EntityID createWith(const Args &... args) {
            const OrderedSignature osig = getOrderedSignature< Args... >();
            const Signature sig(osig.begin(), osig.end());
            rassert(sig.size() == osig.size(), "Duplicate signature", sig.size(), osig.size());

            const EntityID id = nextID++;
            entities[sig].push_back(id);

            (addComponentForID(id, args), ...);

            return id;
        }

        void killEntity(const EntityID id);

        void addSource(SourcePtr &&ptr);
};

}
