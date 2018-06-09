#include "core.h"
#include "mirror.h"

#include "input.h"
#include "renderer.h"
#include "logicManager.h"
#include "entityManager.h"
#include "visualManager.h"
#include "visualManager.h"
#include "entityManager.h"
#include "physicsManager.h"

namespace {

static char *PyCoreArr[][2] = {
    { const_cast< char * >("renderer"), const_cast< char * >("Draws stuff") },
    { const_cast< char * >("input"), const_cast< char * >("Notices stuff") },
    { const_cast< char * >("entities"), const_cast< char * >("Manages stuff") },
    { const_cast< char * >("physics"), const_cast< char * >("Moves stuff") },
    { const_cast< char * >("visuals"), const_cast< char * >("Visualizes stuff") },
    { const_cast< char * >("logic"), const_cast< char * >("Thinks for stuff") },
    { const_cast< char * >("player"), const_cast< char * >("The real enemy") },
    { nullptr, nullptr }
};

static PyStructSequence_Desc PyCore {
    .name = const_cast< char * >("Core"),
    .doc = const_cast< char * >("The core"),
    .fields = reinterpret_cast< PyStructSequence_Field * >(PyCoreArr),
    .n_in_sequence = (sizeof(PyCoreArr) / sizeof(*PyCoreArr)) - 1,
};

static PyTypeObject *k_PyCoreType;

}

RUN_STATIC(
    k_PyCoreType = PyStructSequence_NewType(&PyCore);
    k_PyCoreType->tp_flags |= Py_TPFLAGS_HEAPTYPE;
)

template<>
PyObject *toPython< Core >(Core &core) {
    PyObject *obj = PyStructSequence_New(k_PyCoreType);
    size_t index = 0;
    PyStructSequence_SetItem(obj, index++, toPython(core.renderer));
    PyStructSequence_SetItem(obj, index++, toPython(core.input));
    PyStructSequence_SetItem(obj, index++, toPython(core.entities));
    PyStructSequence_SetItem(obj, index++, toPython(core.physics));
    PyStructSequence_SetItem(obj, index++, toPython(core.visuals));
    PyStructSequence_SetItem(obj, index++, toPython(core.logic));
    PyStructSequence_SetItem(obj, index++, toPython(core.player));
    return obj;
}
