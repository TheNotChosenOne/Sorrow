#pragma once

#include <boost/program_options.hpp>
#include <functional>
#include <optional>
#include <cstddef>
#include <memory>
#include <mutex>
#include <map>

#include <Box2D.h>

#include "utility/utility.h"
#include "core/geometry.h"
#include "core/flags.h"

class Input;
namespace Entity { class Tracker; class SystemManager; }
class Renderer;

struct ThreadedWorld {
    std::mutex tex;
    std::unique_ptr< b2World > b2w;

    void locked(const std::function< void() > &func);
};

struct Core {
    typedef std::map< FlagType, std::unique_ptr< BaseFlag > > FlagMap;

    Input &input;
    Entity::Tracker &tracker;
    Renderer &renderer;
    Entity::SystemManager &systems;
    ThreadedWorld b2world;
    boost::program_options::variables_map options;
    double radius;
    Point camera;
    FlagMap flags;

    double scale() const;

    template < typename T >
    std::optional< std::reference_wrapper< T > > getFlag() {
        const auto loc = flags.find(ctti::type_id< std::remove_const_t< T > >());
        if (flags.end() == loc) { return std::nullopt; }
        return static_cast< Flag< std::remove_const_t< T > > & >(*loc->second).value;
    }

    template < typename T >
    const std::optional< T > getFlag() const {
        const auto loc = flags.find(ctti::type_id< std::remove_const_t< T > >());
        if (flags.end() == loc) { return std::nullopt; }
        return static_cast< const Flag< std::remove_const_t< T > > & >(*loc->second).value;
    }

    template< typename T >
    void setFlag(const T &t) {
        flags[ctti::type_id< std::remove_const_t< T > >()] = std::make_unique< Flag< std::remove_const_t< T > > >(t);
    }

    template < typename T >
    T &ensureFlag() {
        using TypedFlag = Flag< std::remove_const_t< T > >;
        const auto tid = ctti::type_id< std::remove_const_t< T > >();
        const auto loc = flags.find(tid);
        if (flags.end() != loc) { return static_cast< TypedFlag & >(*loc->second).value; }
        setFlag(T());
        rassert(flags.end() != flags.find(tid));
        return static_cast< TypedFlag & >(*flags[tid]).value;
    }
};
