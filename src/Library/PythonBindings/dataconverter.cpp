#include "dataconverter.h"
#include <dynaplex/utilities.h>
#include <unordered_map>
#include <iostream>

namespace DynaPlex {

   
	struct DynaPlex::DataConverter::Impl {
		enum class Type { py_str, py_int, py_float, py_dict, py_list, py_tuple, py_unsupported };

		std::unordered_map<std::string, Type> map;

		Impl() :
			map{ {"<class 'str'>",Type::py_str},{"<class 'int'>",Type::py_int},{"<class 'float'>",Type::py_float},{"<class 'dict'>",Type::py_dict},{"<class 'list'>",Type::py_list},{"<class 'tuple'>",Type::py_tuple} }

		{
		}

		template <typename T>
		T GetValue(py::handle& handle)
		{
			if constexpr (std::is_same_v<T, DynaPlex::Params>)
			{
				auto dict = handle.cast<py::dict>();
				return ToDynaPlexParams(dict);
			}
			else
			{
				try
				{
					T t = handle.cast<T>();
					return t;
				}
				catch (py::cast_error e)
				{
					DynaPlex::Utilities::Fail("Invalid primitive data type - could not convert primitive python value : "s + e.what());
				}
			}
		}

		Type GetType(py::handle& handle)
		{
			std::string type = py::type::of(handle).str();
			if (map.contains(type))
			{
				return map.at(type);
			}
			std::cout << type << std::endl;
			return Type::py_unsupported;
		}

		template <typename dataType, typename ListType >
		void AddVectorType(Type firsttype, ListType listlike, DynaPlex::Params params, std::string key)
		{
			std::vector<dataType> vec{};
			for (py::handle elem : listlike)
			{
				auto type = GetType(elem);
				if (type == firsttype)
				{
					if constexpr (std::is_same_v<dataType, Params::Params>)
					{
						auto conv = elem.cast<py::dict>();
						auto val = ToDynaPlexParams(conv);
						vec.push_back(val);
					}
					else
					{
						auto val = GetValue<dataType>(elem);
						vec.push_back(val);
					}
				}
				else
				{
					DynaPlex::Utilities::Fail("Cannot only process homogeneous lists/tuples, i.e. values must all be of single type (double/int/dict); key:"s + key);
				}
			}
			params.Add(key, vec);
		}


		//Processes 
	    template <typename T>
		void AddListLike(T listlike, DynaPlex::Params params, std::string key)
		{
			if (listlike.size() == 0)
			{//If the list is empty, we need to assume a type. This will be params. 
				Params::ParamsVec val{};
				params.Add(key, val);
			}
			else
			{//Determine the type of the list by looking at first item.					
				py::handle first = listlike[0];
				auto firsttype = GetType(first);
				if (firsttype == Type::py_float)
				{
					AddVectorType<double>(firsttype, listlike, params, key);
				}
				else if (firsttype == Type::py_int)
				{
					AddVectorType<long>(firsttype, listlike, params, key);
				}
				else if (firsttype == Type::py_dict)
				{
					AddVectorType<DynaPlex::Params>(firsttype, listlike, params, key);
				}
				else
				{
					DynaPlex::Utilities::Fail("Lists and tuples are only supported with elements that are dictionary, double, or integer.");
				}
			}
		}


		DynaPlex::Params ToDynaPlexParams(py::dict& dict)
		{
			auto params = DynaPlex::Params{};


			for (auto kv : dict)
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
						auto val = GetValue<DynaPlex::Params>(kv.second);
						params.Add(key, val);
						break;
					}
					case Type::py_tuple:
					{
						auto tuple = kv.second.cast<py::tuple>();
						AddListLike(tuple, params, key);
						break;
					}
					case Type::py_list:
					{
						auto list = kv.second.cast<py::list>();
						AddListLike(list, params, key);
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


	};

	DataConverter::DataConverter() 	{
		pImpl = std::make_shared<Impl>();
	}

	Params DataConverter::ToDynaPlexParams(py::dict& dict)
	{
		return pImpl->ToDynaPlexParams(dict);	
	}


}