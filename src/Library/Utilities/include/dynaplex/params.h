#pragma once
#include <memory>
#include <string>
#include <variant>
#include <map>
#include <vector>
#include <type_traits>


namespace DynaPlex {
	

	class Params
	{
	public:
		using LongVec = std::vector<long>;
		using DoubleVec = std::vector<double>;
		using ParamsVec = std::vector<Params>;
		using DataType = std::variant<long, double, std::string,DynaPlex::Params, LongVec,ParamsVec,DoubleVec>;
		using TupleList = std::initializer_list< std::tuple<std::string, DataType>>;
	    
	
	public:
		Params();

		Params(TupleList list);

		//add IntVec and DoubleVec
		Params& Add(std::string s, int val);
		Params& Add(std::string s, long val);
		Params& Add(std::string s, std::string val);
		Params& Add(std::string s, double val);
		Params& Add(std::string s, LongVec vec);
		Params& Add(std::string s, DoubleVec vec);
		Params& Add(std::string s, Params vec);



		Params& Populate(std::string s, long& out_val);


		void Print();

	private:
		struct Impl;
		std::shared_ptr<Impl> pImpl;

	};
}//namespace DynaPlex