#include <pybind11/pybind11.h>
#include "dynaplex/vargroup.h"
#include "dynaplex/pythonvargroup.h"
#include "dynaplex/errors.h"
#include "dynaplex/neuralnetworktrainer.h"
#include "dynaplex/utilities.h"
#include <torch/torch.h>

namespace py = pybind11;

void process(py::object& obj)
{
	auto pars = DynaPlex::PythonVarGroup(obj);
	pars.Print();
}
void process(py::kwargs& kwargs)
{
	auto& obj = static_cast<py::object&>(kwargs);
	process(obj);
}

void testPytorch()
{
	DynaPlex::NeuralNetworkTrainer trainer{};
	trainer.writeidentifier();
	DynaPlex::VarGroup pars({ {"as",123} });
	pars.Print();
}

py::dict get()
{
	DynaPlex::VarGroup distprops{
			{"type","geom"},
		{"mean",5} };

	return DynaPlex::PythonVarGroup(std::move(distprops));
}


PYBIND11_MODULE(DP_Bindings, m) {
	m.doc() = "DynaPlex extension for Python";
	m.def("get", &get, "gets some parameters");
	m.def("process", py::overload_cast<py::kwargs&>(&process), "Processes kwargs");
	m.def("process", py::overload_cast<py::object&>(&process), "Processes dict");
	m.def("testPyTorch", &testPytorch, "tests pytorch availability");

}