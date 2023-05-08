#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>


namespace DynaPlex {
	

	class Params
	{
	public:
		using IntVec = std::vector<int64_t>;
		using DoubleVec = std::vector<double>;
		using ParamsVec = std::vector<Params>;
		using DataType = std::variant<bool,std::nullptr_t, int64_t, double, std::string,DynaPlex::Params, IntVec,ParamsVec,DoubleVec>;
		using TupleList = std::initializer_list< std::tuple<std::string, DataType>>;
	    
	
	public:
		Params();
		Params(const Params& other);
		Params& operator=(const Params& other);
		~Params();
		Params(Params&& other) noexcept;
		Params& operator=(Params&& other) noexcept;


		Params(TupleList list);
		
		void Add(std::string s, int val);
		void Add(std::string s, int64_t val);
		void Add(std::string s, bool val);
		void Add(std::string s, nullptr_t val);
		void Add(std::string s, std::string val);
		void Add(std::string s, double val);
		void Add(std::string s, const IntVec& vec);
		void Add(std::string s, const DoubleVec& vec);
		void Add(std::string s, const Params& vec);
		void Add(std::string s, const ParamsVec& vec);

		void SaveToFile(const std::string &filename) const;
		static Params LoadFromFile(const std::string &filename);


		void Print();
	private:
		struct Impl;
		std::unique_ptr<Impl> pImpl;

	};
}//namespace DynaPlex