#pragma once
#include <pybind11/pybind11.h>
#include <dynaplex/params.h>
#include <memory>

namespace py = pybind11;


namespace DynaPlex
{
	class DataConverter
	{
	public:
		DataConverter();
		DynaPlex::Params ToDynaPlexParams(py::dict& kwargs);

	private:
		struct Impl;
		std::shared_ptr<Impl> pImpl;
	};
}