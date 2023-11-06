#include <iostream>
#include <fstream>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "vargroup/nlohmann/json.h"
#include "vargroup/vargroup_private_support_funcs.h"//hash_json and check_validity and levenshteinDist
#include <algorithm>
#if DP_PYBIND_SUPPORT
#include "pybind11/pybind11.h"
#include "vargroup/pybind11_json.h"
#endif

namespace DynaPlex {

	using ordered_json = nlohmann::ordered_json;
	class VarGroup::Impl {
	public:


		ordered_json data;

		// Convert a string to lowercase
		std::string toLower(const std::string& s) const{
			std::string result = s;
			std::transform(result.begin(), result.end(), result.begin(), ::tolower);
			return result;
		}

		// This function generates a warning if a similar key is found and returns the similar key (empty if none found).
		std::string FindSimilarKey(const std::string& key) const {
			std::size_t key_length = key.size();



			for (const auto& item : data.items()) {
				const std::string& current_key = item.key();
				if (toLower(current_key) == toLower(key))
				{
					return current_key;
				}
			}

			if (key_length <= 1) {
				return "";  // No warning for single or zero-character keys
			}

			// Set threshold based on key length
			std::size_t similarity_threshold;
			if (key_length <= 6) {
				similarity_threshold = 1;
			}
			else {
				similarity_threshold = 2;
			}

			std::size_t min_distance = std::numeric_limits<std::size_t>::max();
			std::string similar_key;

			// Find the most similar key
			for (const auto& item : data.items()) {
				const std::string& current_key = item.key();
				std::size_t distance = VarGroupHelpers::levenshteinDist(key, current_key);
				if (distance < min_distance && distance <= similarity_threshold) {
					min_distance = distance;
					similar_key = current_key;
				}
			}
			return similar_key;
		}

		bool HasKey(const std::string& key, bool warn_if_similar) const {
			// Check if the key exists directly
			if (data.find(key) != data.end()) {
				return true;
			}
			
			if (warn_if_similar)
			{
				std::string similar_key = FindSimilarKey(key);
				if (!similar_key.empty()) {
					std::cerr << "Warning: Key \"" << key << "\" not found in VarGroup. (A similar key was provided: \"" << similar_key << "\")" << std::endl;					
				}
			}
			return false;
		}

		void AssertKeyExistence(std::string key) const
		{
			if (!data.contains(key)) {
				std::string suggestedKey = FindSimilarKey(key);
				if (suggestedKey.empty()) {
					throw DynaPlex::Error("Key \"" + key + "\" not found in VarGroup.");
				}
				else {
					throw DynaPlex::Error("Key \"" + key + "\" not found in VarGroup. (A similar key was provided: \"" + suggestedKey + "\")");
				}
			}
		}

		std::string PrintAbbrv(const ordered_json& obj, int indent = 0) const {
			std::ostringstream os;
			const std::string indentStr(indent, ' ');

			if (obj.is_object()) {
				os << "{\n";
				for (auto it = obj.begin(); it != obj.end();) {
					os << indentStr << "    \"" << it.key() << "\": ";
					os << PrintAbbrv(it.value(), indent + 4);
					if (++it != obj.end()) {
						os << ",";
					}
					os << "\n";
				}
				os << indentStr << "}";
			}
			else if (obj.is_array()) {
				os << "[";
				if (obj.size() > 5) {
					os << PrintAbbrv(obj[0], indent + 4);
					os << ", ";
					os << PrintAbbrv(obj[1], indent + 4);
					os << ", ... (" << obj.size() - 3 << " omitted) ..., ";
					os << PrintAbbrv(obj[obj.size() - 1], indent + 4);
				}
				else {
					for (auto it = obj.begin(); it != obj.end();) {
						os << PrintAbbrv(*it, indent + 4);
						if (++it != obj.end()) {
							os << ", ";
						}
					}
				}
				os << "]";
			}
			else {
				os << obj.dump();
			}

			return os.str();
		}

		void Add(const std::string& key, const DataType& value, bool override=false) {
			// Check if the key already exists in the data
			if (!override && (data.find(key) != data.end())) {
				throw DynaPlex::Error("Attempted to add a duplicate key: " + key);
			}

			std::visit(
				[this, &key](auto&& v) {
					using T = std::decay_t<decltype(v)>;
					if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, int64_t> || std::is_same_v<T, double> || std::is_same_v<T, std::nullptr_t> || std::is_same_v<T, bool>) {
						data[key] = v;
					}
					else if constexpr (std::is_same_v<T, VarGroup>) {
						data[key] = v.pImpl->data;
					}
					else if constexpr (std::is_same_v<T, Int64Vec> || std::is_same_v<T, DoubleVec> || std::is_same_v<T, StringVec>) {
						data[key] = ordered_json(v);
					}
					else if constexpr (std::is_same_v<T, VarGroupVec>) {
						ordered_json jsonArray;
						for (const auto& p : v) {
							jsonArray.push_back(p.pImpl->data);
						}
						data[key] = jsonArray;
					}
					else
					{
						throw DynaPlex::Error("Unhandled case in VarGroup implementation.");
					}
				},
				value);
		}
		template<typename T>		
		void GetOrDefaultHelper(const std::string& key, T& out_val, const T& default_value)
		{
			if (HasKey(key, true))
			{
				GetHelper(key, out_val);
			}
			else
			{
				out_val = default_value;
			}
		}

		template<typename T>
		void GetHelper(const std::string& key, T& out_val) const {
			
			AssertKeyExistence(key);
			if (!data[key].is_null()) {
				try {
					out_val = data[key].get<T>();
				}
				catch (nlohmann::json::exception& e) {
					throw DynaPlex::Error("Key " + key + " is not of the correct type in VarGroup. Error: " + e.what());
				}
			}
			else {
				out_val = T{};
			}
		}

		void GetVarGroupVec(const std::string& key, VarGroup::VarGroupVec& out_val) {
			AssertKeyExistence(key);

			if (data[key].is_array()) {
				try {
					for (const auto& item : data[key]) {


						if (item.is_object()) {
							VarGroup varGroup = Impl::ToVarGroup(item);
							out_val.push_back(varGroup);
						}
						else if (item.is_null())
						{
							VarGroup varGroup{};
							out_val.push_back(varGroup);
						}
						else {
							throw DynaPlex::Error("Key " + key + " is not of the correct type in VarGroup.");
						}
					}
				}
				catch (nlohmann::json::exception& e) {
					throw DynaPlex::Error("Key " + key + " is not of the correct type in VarGroup. Error: " + e.what());
				}
			}
			else {
				if (data[key].is_null())
				{
					out_val = VarGroupVec{};
				}
				else
					throw DynaPlex::Error("Key " + key + " must be of type array in VarGroup.");
			}
		}

		static VarGroup ToVarGroup(ordered_json json) {

			// Check if the loaded JSON adheres to the homogeneity rule for arrays

			try {
				VarGroup varGroup{};
				varGroup.pImpl->data = json;
				DynaPlex::VarGroupHelpers::check_validity(varGroup.pImpl->data);
				return varGroup;
			}
			catch (const DynaPlex::Error& e)
			{
				throw DynaPlex::Error(std::string("Error in dictionary/kwargs passed from python:\n") + e.what());
			}
			catch (const std::exception& e)
			{
				throw DynaPlex::Error("Error while converting from pybind11::dict to VarGroup");
			}
		}
	};

	VarGroup::VarGroup() : pImpl(std::make_unique<Impl>()) {}


	VarGroup::VarGroup(TupleList list) : VarGroup() {
		for (const auto& [first, second] : list) {
			pImpl->Add(first, second);
		}
	}


	VarGroup::VarGroup(const VarGroup& other) : pImpl(std::make_unique<Impl>(*other.pImpl)) {}

	VarGroup& VarGroup::operator=(const VarGroup& other) {
		if (this != &other) {
			pImpl = std::make_unique<Impl>(*other.pImpl);
		}
		return *this;
	}

	VarGroup::~VarGroup() = default;

	VarGroup::VarGroup(VarGroup&& other) noexcept
		: pImpl(std::move(other.pImpl)) {}

	VarGroup& VarGroup::operator=(VarGroup&& other) noexcept {
		if (this != &other) {
			pImpl = std::move(other.pImpl);
		}
		return *this;
	}

	void VarGroup::Add(std::string s, int val) {
		pImpl->Add(s, static_cast<int64_t>(val));
	}


	void VarGroup::Add(std::string s, int64_t val) {
		pImpl->Add(s, val);
	}
	void VarGroup::Add(std::string s, bool val) {
		pImpl->Add(s, val);
	}


	void VarGroup::Add(std::string s, std::string val) {
		pImpl->Add(s, val);
	}
	void VarGroup::Add(std::string s, const char* val) {
		std::string value = val;
		pImpl->Add(s, value);
	}

	void VarGroup::Add(std::string s, double val) {
		pImpl->Add(s, val);
	}	
	void VarGroup::Add(std::string s, const Int64Vec& vec) {
		pImpl->Add(s, vec);
	}
	void VarGroup::Add(std::string s, const DoubleVec& vec) {
		pImpl->Add(s, vec);
	}
	void VarGroup::Add(std::string s, const StringVec& vec) {
		pImpl->Add(s, vec);
	}
	void VarGroup::Add(std::string s, const VarGroup& vec) {
		pImpl->Add(s, vec);
	}
	void VarGroup::Add(std::string s, const VarGroupVec& vec) {
		pImpl->Add(s, vec);
	}

	void VarGroup::Set(std::string s, int val) {
		pImpl->Add(s, static_cast<int64_t>(val),true);
	}


	void VarGroup::Set(std::string s, int64_t val) {
		pImpl->Add(s, val,true);
	}
	void VarGroup::Set(std::string s, bool val) {
		pImpl->Add(s, val, true);
	}


	void VarGroup::Set(std::string s, std::string val) {
		pImpl->Add(s, val, true);
	}
	void VarGroup::Set(std::string s, const char* val) {
		std::string value = val;
		pImpl->Add(s, value, true);
	}

	void VarGroup::Set(std::string s, double val) {
		pImpl->Add(s, val, true);
	}
	void VarGroup::Set(std::string s, const Int64Vec& vec) {
		pImpl->Add(s, vec, true);
	}
	void VarGroup::Set(std::string s, const DoubleVec& vec) {
		pImpl->Add(s, vec, true);
	}
	void VarGroup::Set(std::string s, const StringVec& vec) {
		pImpl->Add(s, vec, true);
	}
	void VarGroup::Set(std::string s, const VarGroup& vec) {
		pImpl->Add(s, vec, true);
	}
	void VarGroup::Set(std::string s, const VarGroupVec& vec) {
		pImpl->Add(s, vec, true);
	}


	void VarGroup::Get(const std::string& key, int64_t& out_val) const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::GetOrDefault(const std::string& key, int64_t& out_val,const int64_t& default_value) const {
		pImpl->GetOrDefaultHelper(key, out_val, default_value);
	}
	void VarGroup::GetOrDefault(const std::string& key, std::string& out_val, const std::string& default_value) const {
		pImpl->GetOrDefaultHelper(key, out_val, default_value);
	}
	void VarGroup::GetOrDefault(const std::string& key, double& out_val, const double& default_value) const {
		pImpl->GetOrDefaultHelper(key, out_val, default_value);
	}
	void VarGroup::GetOrDefault(const std::string& key, bool& out_val, const bool& default_value) const {
		pImpl->GetOrDefaultHelper(key, out_val, default_value);
	}

	void VarGroup::Get(const std::string& key, std::string& out_val) const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, bool& out_val) const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, double& out_val)const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, VarGroup::Int64Vec& out_val)const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, VarGroup::StringVec& out_val)const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, VarGroup::DoubleVec& out_val)const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, VarGroup::VarGroupVec& out_val)const {
		pImpl->GetVarGroupVec(key, out_val);
	}

	bool VarGroup::HasKey(const std::string& key, bool warn) const
	{
		return pImpl->HasKey(key,warn);
	}

	void VarGroup::Get(const std::string& key, VarGroup& out_val) const {
		pImpl->AssertKeyExistence(key);

		if (pImpl->data[key].is_null()) {
			//empty is interpreted as an empty vargroup. 
			out_val = VarGroup{};
			return;
		}

		if (!pImpl->data[key].is_object()) {
			throw DynaPlex::Error("expected object type for key " + key + ", but found " + std::string(pImpl->data[key].type_name()));
		}

		out_val = Impl::ToVarGroup(pImpl->data[key]);
	}

	std::string VarGroup::ToAbbrvString() const {
		return pImpl->PrintAbbrv(pImpl->data);
	}

	std::string VarGroup::Dump(const int indent) const {
		return pImpl->data.dump(indent);
	}



	std::string VarGroup::UniqueIdentifier() const
	{
		return this->Identifier() + "_" + this->Hash();
	}

	std::string VarGroup::Identifier() const
	{
		std::string id;
		Get("id", id);
		return id;
	}

	void VarGroup::SaveToFile(const std::string& file_path,const int indent) const {
		std::ofstream file(file_path);
		if (!file.is_open()) {
			throw DynaPlex::Error("Failed to open file for writing: " + file_path);
		}
		file << pImpl->data.dump(indent);
		file.close();
	}

	VarGroup VarGroup::LoadFromFile(const std::string& file_path) {
		std::ifstream file(file_path);
		if (file.is_open()) {
			ordered_json j;
			try {
				j= ordered_json::parse(file,
					/* callback */ nullptr,
					/* allow exceptions */ true,
					/* ignore_comments */ true);
			}
			catch (const nlohmann::json::parse_error& e) {
				throw DynaPlex::Error("Failed to parse JSON file: " + file_path + " - " + e.what());
			}
			file.close();

			// Check if the loaded JSON adheres to the homogeneity rule for arrays
			try {
				DynaPlex::VarGroupHelpers::check_validity(j);
			}
			catch (const DynaPlex::Error& e)
			{
				throw DynaPlex::Error(std::string("Error in loaded JSON data from ") + file_path + ":\n  " + e.what());
			}
			VarGroup VarGroup;
			VarGroup.pImpl->data = std::move(j);
			return VarGroup;
		}
		else {
			throw DynaPlex::Error("Unable to open file for reading: " + file_path);
		}
	}

	std::string VarGroup::Hash() const
	{
		return DynaPlex::VarGroupHelpers::hash_json_string(pImpl->data);
	}

	int64_t VarGroup::Int64Hash() const
	{
		return DynaPlex::VarGroupHelpers::hash_json_int64(pImpl->data);
	}


	VarGroup::VarGroup(const std::string& rawJson)
		: pImpl(std::make_unique<Impl>())
	{
		try {
			auto json = nlohmann::ordered_json::parse(rawJson);
			DynaPlex::VarGroupHelpers::check_validity(json);
			pImpl->data = json;
		}
		catch (const nlohmann::json::exception& ex) {
			// Handle the exception appropriately, e.g., throw a custom exception or initialize with an empty JSON
			throw DynaPlex::Error("Failed to parse JSON string: " + std::string(ex.what()));
		}
	}

	bool VarGroup::operator==(const VarGroup& other) const {
		return pImpl->data == other.pImpl->data;
	}

	bool VarGroup::operator!=(const VarGroup& other) const {
		return !(this->operator==(other));
	}


	void VarGroup::SortTopLevel()
	{
		VarGroupHelpers::SortOrderedJson(pImpl->data);
	}



#if DP_PYBIND_SUPPORT
	//We return a unique pointer here (instead of pybind11::dict outright), so that the header does not need to include pybind11 (and can use forward declare).
	// This to avoid longer compilation times as VarGroup.h is needed in many places.  
	std::unique_ptr<pybind11::dict> VarGroup::ToPybind11Dict() const
	{
		if (this->pImpl->data.is_object())
		{
			try
			{
				return std::make_unique<pybind11::dict>(pyjson::from_json(this->pImpl->data));
			}
			catch (const std::exception& e)
			{
				throw DynaPlex::Error(std::string("Could not convert json to dictionary:\n  ") + e.what());
			}
		}
		else
		{
			throw DynaPlex::Error("Cannot convert VarGroup to pybind11::dict.");
		}
	}
	nlohmann::ordered_json ConvertToJson(const pybind11::object& obj)
	{
		try
		{
			py::dict dict = obj.cast<pybind11::dict>();
			nlohmann::ordered_json json = dict;
			return json;
		}
		catch (const pybind11::error_already_set& e) {
			throw DynaPlex::Error(std::string("Invalid Argument: DynaPlex accepts as arguments either a single dictionary or a list of named arguments. The provided argument could not be converted. ") + e.what());
		}
		catch (const std::exception& e)
		{
			throw DynaPlex::Error(std::string("Error while converting argument passed from python to DynaPlex. ") + e.what());
		}
	}


	VarGroup::VarGroup(const pybind11::object& obj) :pImpl(std::make_unique<Impl>())
	{
		auto json = ConvertToJson(obj);
		// Check if the loaded JSON adheres to the homogeneity rule for arrays
		try {
			DynaPlex::VarGroupHelpers::check_validity(json);
		}
		catch (const DynaPlex::Error& e)
		{
			throw DynaPlex::Error(std::string("Error in dictionary/kwargs passed from python:\n") + e.what());
		}
		catch (const std::exception& e)
		{
			throw DynaPlex::Error("Error while converting from pybind11::dict to VarGroup");
		}
		pImpl->data = json;
	}
#endif

}  // namespace DynaPlex
