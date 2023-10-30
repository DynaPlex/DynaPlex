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


	namespace Concepts {
		template<typename T>
		/// <summary>
		/// T is ConvertibleToVarGroup if T has a method ToVarGroup() const that returns a VarGroup
		/// </summary>
		concept ConvertibleToVarGroup = requires(const T & t) {
			{ t.ToVarGroup() } -> std::same_as<VarGroup>;
		};

		/// <summary>
		/// T is VarGroupConvertible if T has a constructor that takes a const VarGroup& as only parameter
		/// </summary>
		template<typename T>
		concept ConvertibleFromVarGroup = requires(const VarGroup & p) {
			{ T(p) };
		};

		/// <summary>
		/// T is VarGroupConvertible it it is both ConvertibleToVarGroup and ConvertibleFromVarGroup
		/// </summary>
		template<typename T>
		concept VarGroupConvertible = ConvertibleToVarGroup<T> && ConvertibleFromVarGroup<T>;


		template<typename T>
		concept DP_BasicElementType = std::is_same_v<T, int64_t> || std::is_same_v<T, double> || std::is_same_v<T, std::string>;

		template<typename T>
		concept DP_ElementType = DP_BasicElementType<T> || VarGroupConvertible<T>;


		template<typename T>
		concept ReadableContainer = requires(T a) {
			typename T::value_type;
				requires std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<decltype(a.begin())>::iterator_category>;
				requires std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<decltype(a.end())>::iterator_category>;
		};



		template<typename T>
		concept ReadableVarGroupContainer = ReadableContainer<T> && ConvertibleToVarGroup<typename T::value_type>;

		template<typename T>
		concept ReadableBasicContainer = ReadableContainer<T> && DP_BasicElementType<typename T::value_type>;


		template<typename T>
		concept AppendableContainer = requires(T a) {
			typename T::value_type;
			{ T() };
			a.clear();
			a.push_back(std::declval<typename T::value_type>());
		};

		template<typename T>
		concept AppendableVarGroupContainer = AppendableContainer<T> && ConvertibleFromVarGroup<typename T::value_type>;

		template<typename T>
		concept AppendableBasicContainer = AppendableContainer<T> && DP_BasicElementType<typename T::value_type>;
	}
	class VarGroup
	{
	public:
		using Int64Vec = std::vector<int64_t>;
		using DoubleVec = std::vector<double>;
		using StringVec = std::vector<std::string>;
		using VarGroupVec = std::vector<VarGroup>;
		using DataType = std::variant<bool, int64_t, double, std::string,DynaPlex::VarGroup, Int64Vec,DoubleVec,StringVec,VarGroupVec>;
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
		///Supports e.g. Add("key", 3); with integer literal - calls Add(std::string s, int64_t val)
		void Add(std::string s, int val);
		void Add(std::string s, int64_t val);
		void Add(std::string s, bool val);
		void Add(std::string s, std::string val);
		void Add(std::string s, const char* val);

		void Add(std::string s, double val);
		void Add(std::string s, const Int64Vec& vec);
		void Add(std::string s, const StringVec& vec);
		void Add(std::string s, const DoubleVec& vec);
		void Add(std::string s, const VarGroupVec& vec);


		void Set(std::string s, const VarGroup& vec);
		///Supports e.g. Add("key", 3); with integer literal - calls Add(std::string s, int64_t val)
		void Set(std::string s, int val);
		void Set(std::string s, int64_t val);
		void Set(std::string s, bool val);
		void Set(std::string s, std::string val);
		void Set(std::string s, const char* val);

		void Set(std::string s, double val);
		void Set(std::string s, const Int64Vec& vec);
		void Set(std::string s, const StringVec& vec);
		void Set(std::string s, const DoubleVec& vec);
		void Set(std::string s, const VarGroupVec& vec);

		template <typename T>
			requires std::is_enum_v<T>
		void Add(const std::string& key,const T& val) {
			using UnderlyingType = std::underlying_type_t<T>;
			static_assert(std::is_same_v<UnderlyingType, int64_t> || std::is_same_v<UnderlyingType, int>,
				"VarGroup: Supported enum class underlying types are int and int64_t only");
			Add(key, static_cast<int64_t>(val));
		}

		template <Concepts::ConvertibleToVarGroup T>
		void Add(const std::string& key,const T& val)
		{
			Add(key, val.ToVarGroup());
		}

		template <Concepts::ReadableVarGroupContainer T>
		void Add(const std::string& key,const T& val)
		{
			VarGroupVec vec;
			for (const auto& item : val) {
				vec.push_back(item.ToVarGroup());
			}
			Add(key, vec);
		}

		template <Concepts::ReadableBasicContainer T>
		void Add(const std::string& key,const T& val)
		{
			std::vector<typename T::value_type> vec;
			for (const auto& item : val) {
				vec.push_back(item);
			}
			Add(key, vec);
		}



		template <typename T>
			requires std::is_enum_v<T>
		void Set(const std::string& key, const T& val) {
			using UnderlyingType = std::underlying_type_t<T>;
			static_assert(std::is_same_v<UnderlyingType, int64_t> || std::is_same_v<UnderlyingType, int>,
				"VarGroup: Supported enum class underlying types are int and int64_t only");
			Set(key, static_cast<int64_t>(val));
		}

		template <Concepts::ConvertibleToVarGroup T>
		void Set(const std::string& key, const T& val)
		{
			Set(key, val.ToVarGroup());
		}

		template <Concepts::ReadableVarGroupContainer T>
		void Set(const std::string& key, const T& val)
		{
			VarGroupVec vec;
			for (const auto& item : val) {
				vec.push_back(item.ToVarGroup());
			}
			Set(key, vec);
		}

		template <Concepts::ReadableBasicContainer T>
		void Set(const std::string& key, const T& val)
		{
			std::vector<typename T::value_type> vec;
			for (const auto& item : val) {
				vec.push_back(item);
			}
			Set(key, vec);
		}


		bool HasKey(const std::string& key, bool warn_if_similar=true) const;


		void GetOrDefault(const std::string& key, int64_t& out_val, const int64_t& default_value) const;
		void GetOrDefault(const std::string& key, double& out_val, const double& default_value) const;
		void GetOrDefault(const std::string& key, bool& out_val, const bool& default_value) const;
		void GetOrDefault(const std::string& key, std::string& out_val, const std::string& default_value) const;

		void Get(const std::string& key, VarGroup& out_val) const;
		void Get(const std::string& key, int64_t& out_val) const;
		void Get(const std::string& key, std::string& out_val) const;
		void Get(const std::string& key, bool& out_val) const;
		void Get(const std::string& key, double& out_val) const;
		void Get(const std::string& key, Int64Vec& out_val) const;
		void Get(const std::string& key, StringVec& out_val) const;
		void Get(const std::string& key, DoubleVec& out_val) const;
		void Get(const std::string& key, VarGroupVec& out_val) const;
		

		template <typename T>
			requires std::is_enum_v<T>
		void Get(const std::string& key, T& out_val) const {
			using UnderlyingType = std::underlying_type_t<T>;
			static_assert(std::is_same_v<UnderlyingType, int64_t> || std::is_same_v<UnderlyingType, int>,
				"VarGroup: Supported enum class underlying types are int and int64_t only");
			int64_t tmpVal;
			Get(key, tmpVal);
			out_val = static_cast<T>(tmpVal);
		}


		
		template<Concepts::ConvertibleFromVarGroup T>
		void Get(const std::string& key, T& out_val) const {
			VarGroup vars;
			Get(key, vars);
			out_val = T(vars);
		}

		template<Concepts::AppendableVarGroupContainer T>
		void Get(const std::string& key, T& out_val) const {
			out_val.clear();

			VarGroupVec varsVec;
			Get(key, varsVec);

			for (VarGroup& p : varsVec) {
				out_val.push_back(typename T::value_type(p));
			}
		}

		template<Concepts::AppendableBasicContainer T>
		void Get(const std::string& key, T& out_val) const {
			out_val.clear();
			using ElementType = typename T::value_type;
			std::vector<ElementType> varsVec;
			Get(key, varsVec);
		
			for (ElementType& p : varsVec) {
				out_val.push_back(p);
			}
		}

		void SaveToFile(const std::string & filePath,const int indent=-1) const;
		static VarGroup LoadFromFile(const std::string &filePath);

		std::string Hash() const;
		int64_t Int64Hash() const;
		std::string ToAbbrvString() const;

		std::string Dump(const int indent = -1) const;


		std::string UniqueIdentifier() const;
		
		std::string Identifier() const;

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