#include "vargroupcaster.h"
#include "gymemulator.h"
#include <pybind11/stl.h> 
#include <pybind11/pybind11.h>

void define_gym_emulator_bindings(py::module_& m) {
    py::class_<DynaPlex::GymEmulator>(m, "gym_emulator")
        .def("reset", [](DynaPlex::GymEmulator& self, py::kwargs kwargs) {
        DynaPlex::VarGroup vargroup(kwargs);
        return self.Reset(vargroup);
        })
        .def("step", &DynaPlex::GymEmulator::Step, py::arg("action"))
        .def("current_state_as_object", &DynaPlex::GymEmulator::CurrentStateAsObject)
        .def("close", &DynaPlex::GymEmulator::Close)
        .def("action_space_size", &DynaPlex::GymEmulator::ActionSpaceSize)
        .def("observation_space_size", &DynaPlex::GymEmulator::ObservationSpaceSize)
        .def("mdp_identifier", &DynaPlex::GymEmulator::GetMDPIdentifier);
}