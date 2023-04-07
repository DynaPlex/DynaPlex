#include "dynaplex/params.h"
#include <iostream>
#include <type_traits>
#include <variant>
#include <map>
#include <stdexcept>

namespace DynaPlex {

	using Map = std::map<std::string, size_t>;
	using TupleVec = std::vector < std::tuple<std::string, Params::DataType>>;
	

	struct Params::Impl {
		Map map;
		TupleVec vec;

	    template <class T>
		void GenericAdd(std::string& s, T& item)
		{
			vec.emplace_back(s, item);
			map[s] = vec.size();
		}


		template <class T>
		static void Print(T& type, int indent)
		{
			auto lambda = [indent](auto& arg) { Params::Impl::Print(arg, indent + 1);	};

			if constexpr (std::is_same_v<T, DynaPlex::Params>)
			{
				bool first = true;
				for (auto& [key, val] : type.pImpl->vec)
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
				int i{ 0 };
				for (DynaPlex::Params& val: type)
				{
					val.pImpl->Print(val, indent);
					if (i++ ==3)
					{
						std::cout << std::endl;
						for (size_t i = 0; i < indent; i++)
						{
							std::cout << "\t";
						}
						std::cout << "...";
						break;
					}
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

		Impl() :map{}, vec{} {}

		Impl(TupleList list) :vec{}, map{}
		{
			for (auto& [key, val] : list)
			{
				if (map.count(key))
				{
					throw std::invalid_argument("Same key present twice in argument list.");
				}
				vec.emplace_back(key, val);
				map[key] = vec.size();
			}
		
		}
	};

	

	Params::Params(TupleList list)
	{		
		pImpl = std::make_shared<Impl>(list);
	}

	Params::Params()
	{
		pImpl = std::make_shared<Impl>();
	}

	void Params::Print()
	{
		pImpl->Print(*this, 0);
		std::cout << std::endl;
	}


	

	Params& Params::Add(std::string s, std::string val)
	{
		pImpl->GenericAdd(s, val);
        return *this;
	}
	Params& Params::Add(std::string s, double val)
	{
		pImpl->GenericAdd(s, val);
		return *this;
	}
	Params& Params::Add(std::string s, int val)
	{
		pImpl->GenericAdd(s, val);
		return *this;
	}
	Params& Params::Add(std::string s, Params::IntVec val)
	{
		pImpl->GenericAdd(s, val);
		return *this;
	}
	Params& Params::Add(std::string s, Params::DoubleVec val)
	{
		pImpl->GenericAdd(s, val);
		return *this;
	}
	Params& Params::Add(std::string s, Params val)
	{
		pImpl->GenericAdd(s, val);
		return *this;
	}
	

}