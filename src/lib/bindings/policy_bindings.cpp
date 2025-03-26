#include "dynaplex/policy.h"
#include "vargroupcaster.h"
#include <pybind11/pybind11.h>

void define_policy_bindings(py::module_& m) {
    py::class_<DynaPlex::PolicyInterface, std::shared_ptr<DynaPlex::PolicyInterface>>(m, "Policy")
        .def("set_action", [](DynaPlex::PolicyInterface& self, DynaPlex::Trajectory& trajectory) {
        self.SetAction({ &trajectory,1 });
            }, pybind11::arg("trajectory"), "uses policy to set the next_action for the trajectory")
        .def("type_identifier", &DynaPlex::PolicyInterface::TypeIdentifier)
        .def("get_config", [](DynaPlex::PolicyInterface& policy) {
        return *(policy.GetConfig().ToPybind11Dict());
            }, "Gets dictionary representing static information for this policy, i.e. the parameters it was configured with.");
}