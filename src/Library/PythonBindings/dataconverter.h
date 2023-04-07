#pragma once
#include <pybind11/pybind11.h>
#include <dynaplex/params.h>
#include <dynaplex/utilities.h>
#include <unordered_map>
#include <iostream>
namespace py = pybind11;


namespace DynaPlex
{
	class DataConverter
	{
	public:
		DataConverter();
		DynaPlex::Params ToDynaPlexParams(py::dict& kwargs);
	private:
		enum class Type { py_str, py_int, py_float ,py_dict, py_unsupported };
		std::unordered_map<std::string, Type> map;

		Type GetType(py::handle& handle);

		template <typename T>
		T GetValue(py::handle& handle)
		{
			try
			{
				T t=handle.cast<T>();
				return t;
			}
			catch (py::cast_error e)
			{
				DynaPlex::Utilities::Fail("Invalid primitive data type - could not convert primitive python value : "s + e.what());
			}
		}
	};
}