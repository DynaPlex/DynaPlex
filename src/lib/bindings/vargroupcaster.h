// VarGroupCaster.h
#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "dynaplex/vargroup.h"

namespace py = pybind11;

// Type caster: Python dict <-> VarGroup
namespace pybind11::detail {
    template <> struct type_caster<DynaPlex::VarGroup> {
    public:
        PYBIND11_TYPE_CASTER(DynaPlex::VarGroup, _("VarGroup"));

        // Python -> C++
        bool load(handle src, bool convert) {
            if (!src)
                return false;
            if (py::isinstance<py::dict>(src)) {
                py::dict d = py::cast<py::dict>(src);
                value = DynaPlex::VarGroup(d);
                return true;
            }
            return false;
        }

        // C++ -> Python
        static handle cast(const DynaPlex::VarGroup& src, return_value_policy /* policy */, handle /* parent */) {
            std::unique_ptr<py::dict> pyDictPtr = src.ToPybind11Dict();
            return (*pyDictPtr).release();
        }
    };
}