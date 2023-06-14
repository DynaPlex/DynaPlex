#include "dynaplex/pythonparams.h"
#include "pybind11_json.h"
#include "json.h"
#include "pybind11/pybind11.h"
#include "dynaplex/errors.h"

namespace DynaPlex
{
	PythonParams::PythonParams(pybind11::dict& dict)
		:Params(dict)
	{		
	}

	



}//namespace DynaPlex