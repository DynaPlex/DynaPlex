#include "dynaplex/mdp.h"
#include "vargroupcaster.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

void define_mdp_numpy_bindings(pybind11::module_& m) {
    auto mdp_class = m.attr("MDP");
    py::class_<DynaPlex::MDPInterface, DynaPlex::MDP>(mdp_class)
        .def("get_features", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
        int64_t dim = self.NumFlatFeatures();
        int64_t first = 1;
        py::array_t<float> features({ first, dim });
        auto r = features.mutable_unchecked<2>();
        std::span<float> feat_view(r.mutable_data(0, 0), dim);
        self.GetFlatFeatures(trajectory.GetState(), feat_view);
        return features;
            }, pybind11::arg("trajectory"), R"docstring(
	returns the features corresponding to a trajectory as a 1*N numpy array, with N being the number of features.
	)docstring")
        .def("get_mask", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
                int64_t dim = self.NumValidActions();
                int64_t first = 1;
                py::array_t<bool> mask({ first, dim });
                auto r = mask.mutable_unchecked<2>();
                std::span<bool> feat_view(r.mutable_data(0, 0), dim);
                for (size_t i = 0; i < dim; i++)
                    feat_view[i] = false;
                self.GetMask({ &trajectory,1 }, feat_view);
                return mask;
            }, pybind11::arg("trajectory"), R"docstring(
	returns the mask corresponding to a trajectory as a 1*M numpy array, with M being the number of valid actions for the MDP.
	)docstring");
}
