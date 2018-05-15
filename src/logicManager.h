#pragma once

#include "componentManager.h"
#include "utility.h"
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

    friend class LogicManager;
};

typedef std::unique_ptr< PythonData > LogicComponent;

class LogicManager: public BaseComponentManager {
    public:
        typedef std::vector< LogicComponent > Components;

    private:
        PyObject *controlFunc;
        Components components;
        Components nursery;

        void create() override;
        void graduate() override;
        void reorder(const std::map< size_t, size_t > &remap) override;
        void cull(size_t count) override;

    public:
        LogicManager();
        ~LogicManager();
        void logicUpdate(Core &core);

        PythonData &get(Entity e);
        const PythonData &get(Entity e) const;
};
