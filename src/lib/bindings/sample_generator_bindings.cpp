#include "dynaplex/samplegenerator.h"
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "vargroupcaster.h"
void define_sample_generator_bindings(pybind11::module_& m) {
    py::class_<DynaPlex::DCL::SampleGenerator>(m, "sample_generator")
		.def("generate_samples", &DynaPlex::DCL::SampleGenerator::GenerateSamples,
			py::arg("policy"),
			py::arg("file_path"),
			"Generates samples using policy (default:random) as rollout policy and stores them in a file at file_path.");

           
}

