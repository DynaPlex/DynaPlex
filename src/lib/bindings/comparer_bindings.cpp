#include "dynaplex/policycomparer.h"
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "vargroupcaster.h"
void define_comparer_bindings(pybind11::module_& m) {
    py::class_<DynaPlex::Utilities::PolicyComparer>(m, "PolicyComparer")
        .def("assess", &DynaPlex::Utilities::PolicyComparer::Assess)
        .def("compare",
            [](DynaPlex::Utilities::PolicyComparer& comparer, DynaPlex::Policy first, DynaPlex::Policy second, int64_t index) -> std::vector<DynaPlex::VarGroup> {
                return comparer.Compare(first, second,index);
            }, py::arg("first"), py::arg("second"), py::arg("index")=-1)
        .def("compare",
            [](DynaPlex::Utilities::PolicyComparer& comparer, std::vector<DynaPlex::Policy> policies, int64_t index) -> std::vector<DynaPlex::VarGroup> {
                return comparer.Compare(policies, index);}
            , py::arg("policies"),py::arg("index")=-1
            );
}