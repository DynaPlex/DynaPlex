#pragma once
#include "dynaplex/params.h"
#include "pybind11/pybind11.h"

namespace DynaPlex {

class PythonParams : public DynaPlex::Params
{
public:
    PythonParams(pybind11::dict& dict);

   // pybind11::dict ToDict()
};

}  // namespace DynaPlex