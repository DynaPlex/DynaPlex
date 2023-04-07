#include "dynaplex/params.h"
#include <iostream>
#include <type_traits>
#include <variant>
#include <unordered_map>
#include <stdexcept>
namespace DynaPlex {

	using Map = std::unordered_map<std::string, size_t>;
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
		void Populate(std::string& key, T& out_val) const
		{
			if (map.count(key) == 0)
			{			
				throw std::invalid_argument(std::string("key '") + key + "' not available in Params");
			}
			auto tuple = vec.at(map.at(key));
			Params::DataType data = std::get<1>(tuple);
			if (std::holds_alternative<T>(data))
			{
				out_val = std::get<T>(data);
			}
			else
			{
				throw std::invalid_argument(std::string("Value corresponding to key '") + key + "' is not of the requested type");
			}
		}


		template <class T>
		static void Print(T& type, int indent)
		{
			auto lambda = [indent](auto& arg) { Params::Impl::Print(arg, indent + 1);	};

			if constexpr (std::is_same_v<T, DynaPlex::Params>)
			{
				bool first = true;
				if (type.pImpl->vec.size() > 0)
				{

					for (auto& [key, val] : type.pImpl->vec)
					{
						if (!first || indent > 0)
						{
							std::cout << std::endl;
						}
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
						std::cout << "\'" << key << "\'" << ":" << "\t";
						std::visit(lambda, val);
					}
					std::cout << "}";
				}
				else
				{
					std::cout << "{empty}";
				}
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
			else if constexpr (std::is_same_v<T, Params::LongVec> || std::is_same_v<T, Params::DoubleVec>) {
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
				map[key] = vec.size();
				vec.emplace_back(key, val);
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
		std::cout  << std::endl;
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
		long l = static_cast<long>(val);
		pImpl->GenericAdd(s, l);
		return *this;
	}
	Params& Params::Add(std::string s, long val)
	{
		pImpl->GenericAdd(s, val);
		return *this;
	}
	Params& Params::Add(std::string s, Params::LongVec val)
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


	Params& Params::Populate(std::string s, long& out_val)
	{
		pImpl->Populate(s, out_val);
		return *this;
	}
	

}