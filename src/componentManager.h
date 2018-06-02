#pragma once

#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>
#include <functional>

typedef size_t Entity;
class EntityManager;

class BaseComponentManager {
    protected:
        // EntityManager tells us to create a new component
        virtual void create() = 0;
        // EntityManager tells us to move newly created entities
        // into the main list
        virtual void graduate() = 0;
        // For each (k, v), move component k to v
        virtual void reorder(const std::map< size_t, size_t > &remap) = 0;
        // Remove the last 'count' components
        virtual void cull(size_t count) = 0;

    public:
        virtual ~BaseComponentManager();

    friend class EntityManager;
};

template< typename T >
class ComponentManager: public BaseComponentManager {
    public:
        typedef T Component;
        typedef std::vector< T > Components;

    protected:
        virtual T makeDefault() const { return T(); }

    private:
        Components components;
        Components previous;
        Components swap;
        Components nursery;

        void create() override {
            nursery.push_back(makeDefault());
        }

        void graduate() override {
            previous.resize(previous.size() + nursery.size());
            std::copy(nursery.begin(), nursery.end(),
                      std::back_inserter(components));
            nursery.clear();
        }

        void reorder(const std::map< size_t, size_t > &remap) override {
            for (const auto &pair : remap) {
                components[pair.second] = std::move(components[pair.first]);
                previous[pair.second] = std::move(previous[pair.first]);
            }
        }

        void cull(size_t count) override {
            components.resize(components.size() - count);
            previous.resize(components.size());
        }

    public:
        typedef std::function< void(const Components &last, Components &next) > Updater;
        virtual ~ComponentManager() { }

        void update(const Updater &updater) {
            swap.resize(components.size());
            updater(components, swap);
            std::swap(previous, components);
            std::swap(swap, components);
            graduate();
        }

        T &get(const Entity entity) {
            if (entity >= components.size()) {
                return nursery[entity - components.size()];
            }
            return components[entity];
        }

        const T &get(const Entity entity) const {
            if (entity >= components.size()) {
                return nursery[entity - components.size()];
            }
            return components[entity];
        }

        T &getLast(const Entity entity) {
            return previous[entity];
        }

        const T &getLast(const Entity entity) const {
            return previous[entity];
        }

    friend class EntityManager;
};
