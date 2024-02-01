#include "dynaplex/policycomparer.h"
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "vargroupcaster.h"


void define_comparer_bindings(pybind11::module_& m) {
    py::class_<DynaPlex::Utilities::PolicyComparer>(m, "PolicyComparer")
        .def("assess", 
            [](DynaPlex::Utilities::PolicyComparer& comparer, DynaPlex::Policy policy) {
                return *(comparer.Assess(policy).ToPybind11Dict());
            }        
        )
        .def("compare",
            [](DynaPlex::Utilities::PolicyComparer& comparer, DynaPlex::Policy first, DynaPlex::Policy second, int64_t index) {
                auto vector_of_vargroup = comparer.Compare(first, second, index);
                py::list list;
                for (auto& vargroup : vector_of_vargroup) 
                    list.append(*vargroup.ToPybind11Dict());
                return list;
            }, py::arg("first"), py::arg("second"), py::arg("index")=-1)
        .def("compare",
            [](DynaPlex::Utilities::PolicyComparer& comparer, std::vector<DynaPlex::Policy> policies, int64_t index) {
                auto vector_of_vargroup = comparer.Compare(policies, index);
                py::list list;               
                for (auto& vargroup : vector_of_vargroup) 
                    list.append(*vargroup.ToPybind11Dict());
                return list;
            }
            , py::arg("policies"),py::arg("index")=-1
            );
}


