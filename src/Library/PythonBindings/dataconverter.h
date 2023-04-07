#pragma once
#include <pybind11/pybind11.h>
#include <dynaplex/params.h>

namespace py = pybind11;


namespace DynaPlex
{
	namespace Converter
	{
		DynaPlex::Params ToDynaPlexParams(py::kwargs& kwargs);
	}
}