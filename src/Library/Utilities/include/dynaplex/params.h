#pragma once
#include <memory>
#include <string>
#include <variant>
#include <map>
#include <vector>
namespace DynaPlex {
	

	class Params
	{
	public:
		using IntVec = std::vector<long>;
		using DoubleVec = std::vector<double>;
		using ParamsVec = std::vector<Params>;
		using DataType = std::variant<int, double, std::string,DynaPlex::Params,IntVec,ParamsVec,DoubleVec>;
		using TupleList = std::initializer_list < std::tuple<std::string, DataType>>;
	    
	
	public:
		Params();

		Params(TupleList map);

		//add IntVec and DoubleVec
		Params& Add(std::string s, int val);
		Params& Add(std::string s, std::string val);
		Params& Add(std::string s, double val);

		void Print();

	private:
		struct Impl;
		std::shared_ptr<Impl> pImpl;

	};
}//namespace DynaPlex