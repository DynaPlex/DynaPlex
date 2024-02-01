#include "dynaplex/trajectory.h"
#include "vargroupcaster.h"
#include <pybind11/stl.h> 
#include <pybind11/pybind11.h>

void define_trajectory_bindings(pybind11::module_& m) {
    pybind11::class_<DynaPlex::Trajectory,std::unique_ptr<DynaPlex::Trajectory>>(m, "Trajectory");

} 
