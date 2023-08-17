#pragma once
#include "dynaplex/vargroup.h"
#include "pybind11/pybind11.h"

namespace DynaPlex {

class PythonVarGroup : public DynaPlex::VarGroup
{

public:
    PythonVarGroup(pybind11::object);
    operator pybind11::dict() const;
    PythonVarGroup(VarGroup&);
    PythonVarGroup(VarGroup&&) noexcept;
};

}  // namespace DynaPlex