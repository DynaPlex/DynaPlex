#include "dynaplex/mdp.h"
#include "vargroupcaster.h"
#include <pybind11/stl.h> 
#include <pybind11/pybind11.h>

void define_mdp_bindings(pybind11::module_& m) {
    pybind11::class_<DynaPlex::MDPInterface, DynaPlex::MDP>(m, "MDP")
        .def("num_valid_actions", &DynaPlex::MDPInterface::NumValidActions)
        .def("provides_flat_features", &DynaPlex::MDPInterface::ProvidesFlatFeatures)
        .def("num_flat_features", &DynaPlex::MDPInterface::NumFlatFeatures)
        .def("identifier", &DynaPlex::MDPInterface::Identifier)
        .def("type_identifier", &DynaPlex::MDPInterface::TypeIdentifier)
        .def("get_static_info", &DynaPlex::MDPInterface::GetStaticInfo)
        .def("discount_factor", &DynaPlex::MDPInterface::DiscountFactor)
        .def("is_infinite_horizon", &DynaPlex::MDPInterface::IsInfiniteHorizon)
        .def("list_policies", &DynaPlex::MDPInterface::ListPolicies)
        .def("get_policy",
            [](DynaPlex::MDPInterface& mdp, py::kwargs& kwargs) {
                // Convert kwargs to DynaPlex::VarGroup
                auto vars = DynaPlex::VarGroup(kwargs);
                return mdp.GetPolicy(vars);
            },
            "Get a policy; supports keyword arguments.")
        .def("get_policy",
            pybind11::overload_cast<const std::string&>(&DynaPlex::MDPInterface::GetPolicy, py::const_),
            pybind11::arg("id"),
            "Convenience function that calls GetPolicy with the parameter id.");

} 
