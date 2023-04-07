#include "dataconverter.h"
#include <dynaplex/utilities.h>

DynaPlex::DataConverter::DataConverter():
    map{{"<class 'str'>",Type::py_str},{"<class 'int'>",Type::py_int},{"<class 'float'>",Type::py_float},{"<class 'dict'>",Type::py_dict} }
{

}



DynaPlex::Params DynaPlex::DataConverter::ToDynaPlexParams(py::dict& kwargs)
{
    auto params = DynaPlex::Params{};


	for (auto kv : kwargs)
	{
		auto type1 = GetType(kv.first);
		auto type2 = GetType(kv.second);
		if (type1 == Type::py_str)
		{
			auto key = GetValue<std::string>(kv.first);

			switch (type2)
			{
			case Type::py_str:
			{
				auto val = GetValue<std::string>(kv.second);
				params.Add(key, val);
				break;
			}	
			case Type::py_float:
			{
				auto val = GetValue<double>(kv.second);
				params.Add(key, val);
				break;
			}
			case Type::py_int:
			{
				auto val = GetValue<long>(kv.second);
				params.Add(key, val);
				break;
			}
			case Type::py_dict:
			{
				auto dict = kv.second.cast<py::dict>();
				auto val = ToDynaPlexParams( dict);
				params.Add(key, val);
				break;
			}
			default:

				break;
			}
		}
		else
		{
			DynaPlex::Utilities::Fail("Error when converting: Only string keys are supported");
		}
	}

	return params;
}

DynaPlex::DataConverter::Type DynaPlex::DataConverter::GetType(py::handle& handle)
{
	std::string type = py::type::of(handle).str();
	if (map.contains(type))
	{
		return map.at(type);
	}	
	std::cout << type << std::endl;
	return Type::py_unsupported;
}
