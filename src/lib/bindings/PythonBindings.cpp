#include <pybind11/pybind11.h>
//#include "dynaplex/params.h"
//#include "dynaplex/pythonparams.h"
//#include "dynaplex/errors.h"
//#include "dynaplex/simple.h"
//#include "dynaplex/neuralnetworktrainer.h"
//#include "dynaplex/utilities.h"
#include <torch/torch.h>

namespace py = pybind11;

void process(py::object& obj)
{
//	auto pars = DynaPlex::PythonParams(obj);
//	pars.Print();
}
void process(py::kwargs& kwargs)
{
//	auto& obj = static_cast<py::object&>(kwargs);
//	process(obj);
}

void testPytorch()
{
#if Torch_available
	std::cout << "DynaPlex: Pytorch available, Version ";
	std::cout << TORCH_VERSION_MAJOR << ".";
	std::cout << TORCH_VERSION_MINOR << ".";
	std::cout << TORCH_VERSION_PATCH << std::endl;
	torch::Tensor eye = torch::eye(2);
	std::cout << eye << std::endl;
	std::cout << torch::cuda::is_available() << std::endl;
#else
	std::cout << "torch not available" << std::endl;
#endif

	//DynaPlex::NeuralNetworkTrainer trainer{};
	//trainer.writeidentifier();
//	DynaPlex::Params pars({ {"as",123} });
//	pars.Print();
}

py::dict get()
{

//	DynaPlex::Params distprops{
//			{"type","geom"},
//		{"mean",5} };

//	return DynaPlex::PythonParams(std::move(distprops));
	return py::dict{};

}


PYBIND11_MODULE(DP_Bindings, m) {
	m.doc() = "DynaPlex extension for Python";
	m.def("get", &get, "gets some parameters");
	m.def("process", py::overload_cast<py::kwargs&>(&process), "Processes kwargs");
	m.def("process", py::overload_cast<py::object&>(&process), "Processes dict");
	m.def("testPyTorch", &testPytorch, "tests pytorch availability");

}