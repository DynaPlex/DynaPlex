#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "vargroupcaster.h"
#include "dynaplex/error.h"
#include "dynaplex/neuralnetworktrainer.h"
#include "dynaplex/utilities.h"
#include <torch/torch.h>
#include <dynaplex/mdp.h>
#include <dynaplex/factories.h>

namespace py = pybind11;

namespace DynaPlex {
	DynaPlex::MDP GetMDP(py::kwargs& kwargs)
	{
		auto vars = DynaPlex::VarGroup(kwargs);
		return DynaPlex::GetMDP(vars);
	}
}

std::string TestParam(DynaPlex::VarGroup& vars)
{
	return vars.ToAbbrvString();
}

std::string TestParam(py::kwargs& kwargs)
{
	auto vars = DynaPlex::VarGroup(kwargs);
	return TestParam(vars );
}

void TestPytorch()
{
	DynaPlex::NeuralNetworkTrainer trainer{};
	trainer.writeidentifier();
}

DynaPlex::VarGroup GetVarGroup()
{
	DynaPlex::VarGroup distprops{
			{"type","geom"},
		{"mean",5} };

	return distprops;
}


PYBIND11_MODULE(DP_Bindings, m) {
	m.doc() = "DynaPlex extension for Python";
	m.def("GetVarGroup", &GetVarGroup, "gets some parameters");
	m.def("TestPytorch", &TestPytorch, "tests pytorch availability");
	// Expose the MDPInterface
	py::class_<DynaPlex::MDPInterface,DynaPlex::MDP>(m, "MDP")
		.def("identifier", &DynaPlex::MDPInterface::Identifier);

	m.def("test_param", py::overload_cast<py::kwargs&>(&TestParam), "simply prints param. ");
	m.def("test_param", py::overload_cast<DynaPlex::VarGroup&>(&TestParam), "simply prints param. ");
	m.def("get_mdp", py::overload_cast<py::kwargs&>(&DynaPlex::GetMDP), "Gets MDP based on dictionary.");
	m.def("get_mdp", py::overload_cast<const DynaPlex::VarGroup&>(&DynaPlex::GetMDP), "Gets MDP based on dictionary.");

}