#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <concepts>

#if DP_PYBIND_SUPPORT 
namespace pybind11 {
	class dict;
	class object;
}
#endif

namespace DynaPlex {

	class VarGroup;


	template<typename T>
	concept ConvertibleFromVarGroup = requires(VarGroup& p) {
		{ T(p) };
	};

	template<typename T>
	concept ConvertibleFromVarGroupVec = requires(T a, typename T::value_type val) {
		typename T::value_type;
		{ T() };
        requires ConvertibleFromVarGroup<typename T::value_type>;
		a.push_back(val);
	};

	class VarGroup
	{
	public:
		using Int64Vec = std::vector<int64_t>;
		using DoubleVec = std::vector<double>;
		using StringVec = std::vector<std::string>;
		using VarGroupVec = std::vector<VarGroup>;
		using DataType = std::variant<bool,std::nullptr_t, int64_t, double, std::string,DynaPlex::VarGroup, Int64Vec,DoubleVec,StringVec,VarGroupVec>;
		using TupleList = std::initializer_list< std::tuple<std::string, DataType>>;

		VarGroup(TupleList list);
		VarGroup(const std::string& rawJson);
		
	    VarGroup();		
		
		VarGroup(const VarGroup& other);
		VarGroup& operator=(const VarGroup& other);
		~VarGroup();
		VarGroup(VarGroup&& other) noexcept;
		VarGroup& operator=(VarGroup&& other) noexcept;

		bool operator==(const VarGroup& other) const;
		bool operator!=(const VarGroup& other) const;

		void Add(std::string s, const VarGroup& vec);
		void Add(std::string s, int val);
		void Add(std::string s, int64_t val);
		void Add(std::string s, bool val);
		void Add(std::string s, std::string val);
		void Add(std::string s, const char* val);

		void Add(std::string s, double val);
		void Add(std::string s, const Int64Vec& vec);
		void Add(std::string s, const std::vector<int>& vec);
		void Add(std::string s, const StringVec& vec);
		void Add(std::string s, const DoubleVec& vec);
		void Add(std::string s, const VarGroupVec& vec);



		void Get(const std::string& key, VarGroup& out_val) const;
		void Get(const std::string& key, int64_t& out_val) const;
		void Get(const std::string& key, std::string& out_val) const;
		void Get(const std::string& key, int& out_val) const;
		void Get(const std::string& key, bool& out_val) const;
		void Get(const std::string& key, double& out_val) const;
		void Get(const std::string& key, Int64Vec& out_val) const;
		void Get(const std::string& key, StringVec& out_val) const;
		void Get(const std::string& key, DoubleVec& out_val) const;
		void Get(const std::string& key, VarGroupVec& out_val) const;
		void Get(const std::string& key, std::vector<int>& out_val) const;
		
		
		template<ConvertibleFromVarGroup T>
		void Get(const std::string& key, T& out_val) const {
			VarGroup vars;
			Get(key, vars);
			out_val = T(vars);
		}



		template<ConvertibleFromVarGroupVec T>
		void Get(const std::string& key, T& out_val) const {
			out_val.clear();

			VarGroupVec varsVec;
			Get(key, varsVec);

			for (VarGroup& p : varsVec) {
				out_val.push_back(typename T::value_type(p));
			}
		}

		void SaveToFile(const std::string &filename) const;
		static VarGroup LoadFromFile(const std::string &filename);

		std::string Hash() const;
		std::string ToAbbrvString() const;

		
		//sorts the top level of the VarGroup by key, in alphabetical order.
		void SortTopLevel();
		

#if DP_PYBIND_SUPPORT
		std::unique_ptr<pybind11::dict> ToPybind11Dict() const;
		VarGroup(const pybind11::object&);
#endif

	private:
		struct Impl;
		std::unique_ptr<Impl> pImpl;

	};
}//namespace DynaPlex