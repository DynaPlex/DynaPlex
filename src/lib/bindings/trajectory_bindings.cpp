#include "dynaplex/trajectory.h"
#include "vargroupcaster.h"
#include <pybind11/stl.h> 
#include <pybind11/pybind11.h>

void define_trajectory_bindings(pybind11::module_& m) {
    pybind11::class_<DynaPlex::Trajectory>(m, "trajectory")
        .def("state_as_dict", [](DynaPlex::Trajectory& self) {
        if (self.HasState())
            return *(self.GetState()->ToVarGroup().ToPybind11Dict());
        else
            throw DynaPlex::Error("trajectory.state_as_dict - trajectory is uninitialized and does not have underlying state.");
            },
            "Converts the state underlying this trajectory to a python dictionary. Note: relatively expensive operation.")
        .def_readwrite("next_action", &DynaPlex::Trajectory::NextAction)
        .def_readonly("external_index", &DynaPlex::Trajectory::ExternalIndex)
        .def_readonly("period_count", &DynaPlex::Trajectory::PeriodCount)
        .def_readonly("effective_discount_factor", &DynaPlex::Trajectory::EffectiveDiscountFactor)
        .def_readonly("cumulative_return", &DynaPlex::Trajectory::CumulativeReturn)
        .def("seed_rngprovider", [](DynaPlex::Trajectory& self, int64_t sample_seed,int64_t trajectory_seed) {
                //should eventually make one system_wide RNG_SEED. For now, use:
                int64_t rng_seed = 26071983;
                self.RNGProvider.SeedEventStreams(rng_seed, false, sample_seed, trajectory_seed);
            }, py::arg("sample_seed") = (1ll << 30) - 1, py::arg("trajectory_seed") = (1ll << 22) - 1)
        .def("is_final", [](DynaPlex::Trajectory& traj) {return traj.Category.IsFinal(); })
        .def("awaits_event", [](DynaPlex::Trajectory& traj) {return traj.Category.IsAwaitEvent(); })
        .def("awaits_action", [](DynaPlex::Trajectory& traj) {return traj.Category.IsAwaitAction(); })
        .def("category_index", [](DynaPlex::Trajectory& traj) {return traj.Category.Index();});
} 
