#include "dynaplex/params.h"
#include <iostream>
#include <type_traits>
#include <variant>
#include <map>
#include <stdexcept>

namespace DynaPlex {

	using Map = std::map<std::string, Params::DataType>;

	

	struct Params::Impl {
		Map map;

		template <class T>
		static void Process(T& type, int indent)
		{
			auto lambda = [indent](auto& arg) { Params::Impl::Process(arg, indent + 1);	};

			if constexpr (std::is_same_v<T, DynaPlex::Params>)
			{
				bool first = true;
				for (auto& [key, val] : type.pImpl->map)
				{			
					std::cout << std::endl;
			
					for (size_t i = 0; i < indent; i++)
					{
						std::cout << "\t";
					}	
					if (first)
					{
						std::cout << "{";
						first = false;
					}
					else
					{
						std::cout << " ";
					}
					std::cout<< "\'" << key << "\'" << ":" << "\t";
					std::visit(lambda, val);
				}

				std::cout << "}";
			}
			else if constexpr (std::is_same_v<T, Params::ParamsVec> ) {
				std::cout << "(";
				for (DynaPlex::Params& val: type)
				{
					val.pImpl->Process(val, indent);
				}
				std::cout << ")";
			}			
			else if constexpr (std::is_same_v<T, Params::IntVec> || std::is_same_v<T, Params::DoubleVec>) {
				std::cout << "(";
				size_t i{ 0 };
				for (auto val : type)
				{
					std::cout << val;
					if (++i < type.size())
					{
						std::cout << ", ";
						if (i > 6)
						{
							std::cout << ".... )";
							break;
						}						
					}
					else
					{
						std::cout << ")";
					}
				}
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				std::cout << "\'" << type << "\'";
			}else{				
					std::cout << type;
			}			
		}

		Impl() :map{} {}

		Impl(Map map) :map{ map } {}
	};

	

	Params::Params(TupleList list)
	{
		Map map{};
		for (auto& [key, val] : list)
		{
			if (map.count(key))
			{
				throw std::invalid_argument("Same key present twice in argument list.");
			}
			map[key] = val;
		}
		pImpl = std::make_shared<Impl>(std::move(map));
	}

	Params::Params()
	{
		Map map{};
		pImpl = std::make_shared<Impl>(std::move(map));
	}

	void Params::Print()
	{
		pImpl->Process(*this, 0);
		std::cout << std::endl;
	}


	template <class T>
	void GenericAdd(Map& map, std::string& s, T& item)
	{
		map[s] = item;
	}

	Params& Params::Add(std::string s, std::string val)
	{
		GenericAdd(pImpl->map, s, val);
        return *this;
	}
	Params& Params::Add(std::string s, double val)
	{
		GenericAdd(pImpl->map, s, val);
		return *this;
	}

	

}