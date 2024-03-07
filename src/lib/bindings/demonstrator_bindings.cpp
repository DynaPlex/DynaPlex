#include "dynaplex/demonstrator.h"
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "vargroupcaster.h"


void define_demonstrator_bindings(pybind11::module_& m) {
    py::class_<DynaPlex::Utilities::Demonstrator>(m, "demonstrator")
        .def("get_trace",
            [](DynaPlex::Utilities::Demonstrator& demonstrator, DynaPlex::MDP mdp, DynaPlex::Policy policy) {
                auto trace = demonstrator.GetTrace(mdp,policy);
                std::vector<py::dict> return_val{};
                return_val.reserve(trace.size());
                for (auto& vg : trace)
                {
                    return_val.push_back(*vg.ToPybind11Dict());
                }
                return return_val;
            },py::arg("mdp"),py::arg("policy")=nullptr,"gets a trace for demonstration and rendering purposes."
        )
        ;
}


