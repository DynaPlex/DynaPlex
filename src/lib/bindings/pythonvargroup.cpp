#include "dynaplex/pythonvargroup.h"
#include "pybind11_json/pybind11_json.h"
#include "nlohmann/json.h"
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

	PythonVarGroup::PythonVarGroup(pybind11::object object)	
		:VarGroup(ConvertToJson(object))
	{		
		
	}

	PythonVarGroup::operator pybind11::dict() const
	{
		try
		{
			pybind11::dict dict= DynaPlex::VarGroup::ToJson();
			return dict;
		}
		catch (const std::exception& e)
		{
			throw DynaPlex::Error(std::string("Could not convert json to dictionary:\n  ") + e.what());
		}
	}	

	PythonVarGroup::PythonVarGroup(VarGroup& vars)
		:VarGroup(vars)
	{
	}

	PythonVarGroup::PythonVarGroup(VarGroup&& other) noexcept
		:VarGroup(std::move(other))
	{
	}


	



}//namespace DynaPlex