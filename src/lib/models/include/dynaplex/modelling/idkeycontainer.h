#pragma once
#include <vector>
#include <optional>
#include "dynaplex/error.h"
#include "dynaplex/vargroup.h"
#include <string>
#include <algorithm>

namespace DynaPlex {

	template<typename T>
	concept HasInt64Index = requires (T t) {
		{ t.index } -> std::same_as<int64_t&>;
	};

	template<typename T>
	concept HasStringKey = requires (T t) {
		{ t.key } -> std::same_as<std::string&>;
	};

	/**
	 * IdKeyContainer is specifically suitable if T can be constructed from VarGroup, and if we are provided
	 * a list of objects that each have a public std::string key. The container constructs the objects, and makes them
	 * accessibly either by index, or by key. 
	 */
	template<Concepts::ConvertibleFromVarGroup T>
	requires HasStringKey<T>
	class IdKeyContainer {
	private:
		std::vector<T> backstore;
		std::vector<std::string> keys;
		std::unordered_map<std::string, int64_t> key_lookup;

	public:
		using value_type = T;
		using iterator = std::vector<T>::iterator;
		using const_iterator = std::vector<T>::const_iterator;

		const std::string& key_for(int64_t index) const
		{
			return keys.at(index);
		}

		int64_t index_for(const std::string& key) const{
			// Check if the key exists in the map
			auto it = key_lookup.find(key);
			if (it != key_lookup.end()) {
				// Key found, return the corresponding value
				return it->second;
			}
			else {
				// Key not found, handle it accordingly
				// Here, I throw an exception, but you could return a default value or handle it differently
				throw DynaPlex::Error("IdKeyContainer: Key \'"+ key + "\' not found");
			}
		}

		IdKeyContainer() {}

		//for backward compatibility. To use this, key need not be present as
		//it will be filled in. 
		static IdKeyContainer<T> FromObjectVarGroup(const VarGroup& varGroup)
		{
			IdKeyContainer<T> container;
			auto keys = varGroup.Keys();
			container.backstore.reserve(keys.size());
			container.key_lookup.reserve(keys.size());
			container.keys.reserve(keys.size());

			int64_t index{ 0 };

			for (const auto& key : keys) {
				DynaPlex::VarGroup vg;
				varGroup.Get(key, vg);
				container.backstore.emplace(vg);
				if constexpr (HasInt64Index<T>)
				{
					container.backstore.back().index = index;
				}
				if constexpr (HasStringKey<T>)
				{
					container.backstore.back().key = key;
				}
				container.keys.push_back(key);
				container.key_lookup[key] = index++;
			}
			return container;
		}
		void reserve(size_t size)
		{
			keys.reserve(size);
			key_lookup.reserve(size);
			backstore.reserve(size);
		}
		
		void push_back(const DynaPlex::VarGroup& vg) {
			backstore.emplace_back(vg);
			key_lookup[backstore.back().key] = keys.size();
			if constexpr (HasInt64Index<T>)
			{
				backstore.back().index = keys.size();
			}
			keys.push_back(backstore.back().key);

		}

		const T& operator[](int64_t id) const {
			return backstore.at(id);
		}

		T& operator[](int64_t id) {
			return backstore.at(id);
		}

		size_t size() const {
			return backstore.size();
		}

		void clear() {
			backstore.clear();
			keys.clear();
			key_lookup.clear();
		}


		iterator begin() {
			return backstore.begin();
		}

		iterator end() {
			return backstore.end();
		}

		const_iterator begin() const {
			return backstore.begin();
		}

		const_iterator end() const {
			return backstore.end();
		}
	};

} // namespace DynaPlex