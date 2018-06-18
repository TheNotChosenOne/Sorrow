#pragma once

#include "componentManager.h"
#include "forwardMirror.h"
#include "utility.h"
#include "timers.h"
#include "input.h"
#include "core.h"

#include <Python.h>

#include <memory>
#include <string>
#include <map>

typedef struct _object PyObject;

class PythonData {
    private:
        PyObject *dict;

    public:
        PythonData();
        ~PythonData();

        void setBool(const std::string &key, bool val);
        void setInt(const std::string &key, int64_t val);
        void setDouble(const std::string &key, double val);
        void setString(const std::string &key, const std::string &val);

        bool getBool(const std::string &key);
        int64_t getInt(const std::string &key);
        double getDouble(const std::string &key);
        std::string getString(const std::string &key);

        bool has(const std::string &key);

    friend class LogicManager;
    friend PyObject *toPython< PythonData >(PythonData &p);
    friend std::ostream &operator<<(std::ostream &, const PythonData &);
};
typedef PythonData LogicComponent;

std::ostream &operator<<(std::ostream &os, const PythonData &pd);

template<>
PyObject *toPython< PythonData >(PythonData &p);

class LogicManager: public BaseComponentManager {
    public:
        typedef std::vector< std::unique_ptr< LogicComponent > > Components;

    private:
        std::map< std::string, PyObject * > controlFuncs;
        std::map< std::string, std::set< size_t > > groups;
        PyObject *pyCore;
        ActionTimer perfTimer;
        bool perfActive;

        Components components;
        Components nursery;

        void create() override;
        void graduate() override;
        void reorder(const std::map< size_t, size_t > &remap) override;
        void cull(size_t count) override;

    public:
        Core *core;
        explicit LogicManager(const boost::program_options::variables_map &options);
        ~LogicManager();
        void setup(Core &core);
        void logicUpdate(Core &core);

        PythonData &get(Entity e);
        const PythonData &get(Entity e) const;
        const std::set< size_t > &getGroup(const std::string &name) const;
};

template<>
PyObject *toPython< LogicManager >(LogicManager &lm);
