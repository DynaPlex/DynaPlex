#include "dynaplex/pythonparams.h"
#include "dynaplex/pybind11_json.h"
#include "dynaplex/json.h"
#include "pybind11/pybind11.h"
#include "dynaplex/errors.h"

namespace DynaPlex
{

	nlohmann::ordered_json ConvertToJson(pybind11::object& obj)
	{
		try
		{
			py::dict dict = obj.cast<pybind11::dict>();
			nlohmann::ordered_json json = dict;
			return json;
		}
		catch (const pybind11::error_already_set& e) {
			throw DynaPlex::Error(std::string("Invalid Argument: DynaPlex accepts as arguments either a single dictionary or a list of named arguments. The provided argument could not be converted. ") + e.what());
		}
		catch (const std::exception& e)
		{
			throw DynaPlex::Error(std::string("Error while converting argument passed from python to DynaPlex. ") + e.what());
		}
	}

	PythonParams::PythonParams(pybind11::object object)	
		:Params(ConvertToJson(object))
	{		
		
	}

	PythonParams::operator pybind11::dict() const
	{
		try
		{
			pybind11::dict dict= DynaPlex::Params::ToJson();
			return dict;
		}
		catch (const std::exception& e)
		{
			throw DynaPlex::Error(std::string("Could not convert json to dictionary:\n  ") + e.what());
		}
	}	

	PythonParams::PythonParams(Params& params)
		:Params(params)
	{
	}

	PythonParams::PythonParams(Params&& other) noexcept
		:Params(std::move(other))
	{
	}


	



}//namespace DynaPlex