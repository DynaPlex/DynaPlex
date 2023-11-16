#include "dynaplex/dynaplexprovider.h"
#include "vargroupcaster.h"
#include <pybind11/pybind11.h>
#include "gymemulator.h"
#include "dynaplex/samplegenerator.h"

namespace DynaPlex {

	DynaPlex::GymEmulator GetGymEmulator(DynaPlex::MDP mdp, py::kwargs& kwargs)
	{
		auto vars = DynaPlex::VarGroup(kwargs);
		return DynaPlex::GymEmulator(DynaPlex::DynaPlexProvider::Get().System(), mdp, vars);
	}

	DynaPlex::MDP GetMDP(py::kwargs& kwargs) {
		return DynaPlex::DynaPlexProvider::Get().GetMDP(kwargs);
	}

	DynaPlex::VarGroup ListMDPs()
	{
		return DynaPlex::DynaPlexProvider::Get().ListMDPs();
	}
	DynaPlex::Policy LoadPolicy(DynaPlex::MDP mdp, std::string path)
	{
		return DynaPlex::DynaPlexProvider::Get().LoadPolicy(mdp, path);
	}

	std::string filepath(const py::args& subdirs_list)
	{
		if (subdirs_list.empty()) {
			throw std::invalid_argument("The list cannot be empty.");
		}

		std::vector<std::string> subdirs_vector;
		// Reserve the last element of the list to be the filename
		const auto& item = subdirs_list[subdirs_list.size() - 1];
		if (!py::isinstance<py::str>(item)) {
			throw std::invalid_argument("All items in the list must be strings.");
		}
		std::string filename = item.cast<std::string>();

		// Iterate through the list until the second-to-last element
		for (py::ssize_t i = 0; i < subdirs_list.size() - 1; ++i) {
			const auto& item = subdirs_list[i];
			if (py::isinstance<py::str>(item)) {
				subdirs_vector.push_back(item.cast<std::string>());
			}
			else {
				throw std::invalid_argument("All items in the list must be strings.");
			}
		}

		// Pass the vector of subdirectories and the filename to the function
		return DynaPlex::DynaPlexProvider::Get().FilePath(subdirs_vector, filename);
	}

	std::string IO_Path()
	{
		auto& system = DynaPlex::DynaPlexProvider::Get().System();
		if (!system.HasIODirectory())
			throw DynaPlex::Error("io_path : Dynaplex does not have IO directory configured.");
		return system.IOLocation();
	}

	DynaPlex::Utilities::PolicyComparer GetComparer(DynaPlex::MDP mdp, py::kwargs& kwargs)
	{
		return DynaPlex::DynaPlexProvider::Get().GetPolicyComparer(mdp, kwargs);
	}
	//define bindings for this. 
	DynaPlex::Algorithms::DCL GetDCL(DynaPlex::MDP mdp, DynaPlex::Policy policy, py::kwargs& kwargs) {
		return DynaPlex::DynaPlexProvider::Get().GetDCL(mdp, policy, kwargs);
	}

	DynaPlex::DCL::SampleGenerator GetSampleGenerator(DynaPlex::MDP mdp, py::kwargs& kwargs) {
		auto& system = DynaPlex::DynaPlexProvider::Get().System();
		return DynaPlex::DCL::SampleGenerator(system, mdp, kwargs);
	}
}


void define_provider_bindings(pybind11::module_& m) {
	m.def("list_mdps", &DynaPlex::ListMDPs, "Lists available MDPs");
	m.def("load_policy", &DynaPlex::LoadPolicy, py::arg("mdp"), py::arg("path"), "loads policy for mdp from path");
	m.def("get_mdp", &DynaPlex::GetMDP, "Gets MDP based on keyword arguments.");
	m.def("get_comparer", &DynaPlex::GetComparer, py::arg("mdp"), "Gets comparer based on MDP and keyword arguments.");
	m.def("io_path", &DynaPlex::IO_Path, "Gets the path of the dynaplex IO directory.");
	m.def("get_gym_emulator", &DynaPlex::GetGymEmulator, py::arg("mdp"), "Gets comparer based on MDP and keyword arguments.");
	m.def("get_dcl", &DynaPlex::GetDCL,
		py::arg("mdp"),
		py::arg("policy") = nullptr,
		"Returns a class that can be used to run dcl algorithm based on mdp, policy and keyword arguments.");
	m.def("get_sample_generator", &DynaPlex::GetSampleGenerator,
		py::arg("mdp"),
		"Returns a class that can be used to generate roll-out samples for a specific mdp.");
	m.def("filepath", &DynaPlex::filepath,
		"Constructs a file path from a list of subdirectories and a filename, which is assumed to be the last element of the list. Creates the directory if not existent, but does not verify or require the existence of the file.");
}
