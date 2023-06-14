#include <pybind11/pybind11.h>
#include "dynaplex/pythonparams.h"
#include "dynaplex/errors.h"


namespace py = pybind11;

void process(py::dict dict)
{
    try {
        auto pars = DynaPlex::PythonParams(dict);
         pars.Print();

    }
    catch (const DynaPlex::Error& e)
    {
        py::pybind11_fail(e.what());
    }    
}

void process(py::kwargs kwargs)
{
    auto dict = static_cast<py::dict>(kwargs);
    process(dict);
}


PYBIND11_MODULE(DynaPlex, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    m.def("process",py::overload_cast<py::kwargs>(&process), "Processes kwargs");
    m.def("process", py::overload_cast<py::dict>(&process), "Processes dict");
   
}