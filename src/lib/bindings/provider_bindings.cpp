#include "dynaplex/dynaplexprovider.h"
#include "vargroupcaster.h"
#include <pybind11/pybind11.h>
#include "gymemulator.h"

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
		return DynaPlex::DynaPlexProvider::Get().GetDCL(mdp,policy,kwargs);
		return DynaPlex::DynaPlexProvider::Get().GetDCL(mdp, policy, kwargs);
	}
}


void define_provider_bindings(pybind11::module_& m) {
	m.def("list_mdps", &DynaPlex::ListMDPs, "Lists available MDPs");
	m.def("load_policy", &DynaPlex::LoadPolicy, py::arg("mdp"), py::arg("path"), "loads policy for mdp from path");
	m.def("get_mdp", &DynaPlex::GetMDP, "Gets MDP based on keyword arguments.");
	m.def("get_comparer", &DynaPlex::GetComparer, py::arg("mdp"), "Gets comparer based on MDP and keyword arguments.");
	m.def("io_path", &DynaPlex::IO_Path, "Gets the path of the dynaplex IO directory.");
	m.def("get_dcl", &DynaPlex::GetDCL, py::arg("mdp"), py::arg("policy"));
	m.def("get_gym_emulator", &DynaPlex::GetGymEmulator, py::arg("mdp"), "Gets comparer based on MDP and keyword arguments.");
	m.def("get_dcl", &DynaPlex::GetDCL,
		py::arg("mdp"),
		py::arg("policy") = nullptr,
		"Returns a class that can be used to run dcl algorithm based on mdp, policy and keyword arguments.");
}
