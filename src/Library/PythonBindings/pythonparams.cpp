#include "dynaplex/pythonparams.h"
#include "pybind11_json.h"
#include "json.h"
#include "pybind11/pybind11.h"
#include "dynaplex/errors.h"

namespace DynaPlex
{

	nlohmann::ordered_json ConvertToJson(pybind11::dict& dict)
	{
		try
		{
			nlohmann::ordered_json json = dict;
			return json;
		}
		catch (const std::exception& e)
		{
			throw DynaPlex::Error(std::string("Could not convert dictionary to ordered json:\n  ") + e.what());
		}
	}

	PythonParams::PythonParams(pybind11::dict dict)	
		:Params( ConvertToJson(dict) )
	{		
	}

	PythonParams::operator pybind11::dict() const
	{
		try
		{
			pybind11::dict dict = ToJson();
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


	



}//namespace DynaPlex