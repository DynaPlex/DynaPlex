#include "dynaplex/policy.h"
#include "vargroupcaster.h"
#include <pybind11/pybind11.h>

void define_policy_bindings(py::module_& m) {
    py::class_<DynaPlex::PolicyInterface, std::shared_ptr<DynaPlex::PolicyInterface>>(m, "Policy")
        .def("type_identifier", &DynaPlex::PolicyInterface::TypeIdentifier)
        .def("get_config", &DynaPlex::PolicyInterface::GetConfig);
}