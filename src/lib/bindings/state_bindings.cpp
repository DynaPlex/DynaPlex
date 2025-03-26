#include "dynaplex/state.h"
#include "vargroupcaster.h"
#include <pybind11/stl.h> 
#include <pybind11/pybind11.h>




void define_state_bindings(pybind11::module_& m) {
    pybind11::class_<DynaPlex::StateBase, DynaPlex::dp_State>(m, "State");
} 
