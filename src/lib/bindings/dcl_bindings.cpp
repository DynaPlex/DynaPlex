#include "dynaplex/dcl.h"
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "vargroupcaster.h"
void define_dcl_bindings(pybind11::module_& m) {
        py::class_<DynaPlex::Algorithms::DCL>(m, "DCL")
            .def("train_policy", [](DynaPlex::Algorithms::DCL& dcl) {
                py::gil_scoped_release release;
                dcl.TrainPolicy();
                })
            .def("get_policy", &DynaPlex::Algorithms::DCL::GetPolicy, py::arg("generation") = -1)
            .def("get_policies", &DynaPlex::Algorithms::DCL::GetPolicies)
            .def("generate_feats_samples", &DynaPlex::Algorithms::DCL::GenerateFeatsSamples)
            .def("get_path_of_feats_sample_file", &DynaPlex::Algorithms::DCL::GetPathOfFeatsSampleFile)
            .def("get_paths_of_policy_files", &DynaPlex::Algorithms::DCL::GetPathsOfPolicyFiles);
 }

