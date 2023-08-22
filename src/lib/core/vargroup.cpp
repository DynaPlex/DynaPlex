#include "dynaplex/vargroup.h"
#include "dynaplex/utilities.h"
#include <iostream>
#include <fstream>
#include "dynaplex/error.h"
#include "vargroup/nlohmann/json.h"
#include "vargroup/vargroup_helpers.h"//hash_json and check_validity
#if DP_PYBIND_SUPPORT
#include "pybind11/pybind11.h"
#include "vargroup/pybind11_json.h"
#endif

namespace DynaPlex {

	using ordered_json = nlohmann::ordered_json;
	class VarGroup::Impl {
	public:


		ordered_json data;

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

		void Add(const std::string& key, const DataType& value) {
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

		void AssertKeyExistence(std::string key) const
		{
			if (!data.contains(key)) {				
				throw DynaPlex::Error("Key \"" + key + "\" not found in VarGroup.");
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
	void VarGroup::Add(std::string s, const std::vector<int>& vec) {
		std::vector<int64_t> vec64(vec.begin(), vec.end());
		pImpl->Add(s, vec64);
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
	void VarGroup::Get(const std::string& key, int64_t& out_val) const {
		pImpl->GetHelper(key, out_val);
	}
	void VarGroup::Get(const std::string& key, std::string& out_val) const {
		pImpl->GetHelper(key, out_val);
	}

	void VarGroup::Get(const std::string& key, int& out_val) const {
		int64_t int64;
		pImpl->GetHelper(key, int64);

		if (int64 < INT_MIN || int64 > INT_MAX) {
			throw DynaPlex::Error("int64_t value out of range for conversion to int");
		}
		out_val = static_cast<int>(int64);
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

	void VarGroup::Get(const std::string& key, std::vector<int>& out_val) const {
		std::vector<int64_t> tmp;
		pImpl->GetHelper(key, tmp);


		out_val.clear();
		for (const auto& val : tmp) {
			if (val < std::numeric_limits<int>::min() || val > std::numeric_limits<int>::max()) {
				throw DynaPlex::Error("Cannot convert to std::vector<int>. Value " + std::to_string(val) + " for key " + key + " cannot be represented as int.");
			}
			out_val.push_back(static_cast<int>(val));
		}
	}


	void VarGroup::Get(const std::string& key, VarGroup& out_val) const {
		pImpl->AssertKeyExistence(key);

		if (!pImpl->data[key].is_object()) {
			throw DynaPlex::Error("expected object type for key " + key + ", but found " + std::string(pImpl->data[key].type_name()));
		}

		out_val = Impl::ToVarGroup(pImpl->data[key]);
	}

	std::string VarGroup::ToAbbrvString() const {
		return pImpl->PrintAbbrv(pImpl->data);
	}

	std::string VarGroup::Identifier() const
	{
		std::string id;
		Get("id", id);
		return id+"_"+this->Hash();
	}

	void VarGroup::SaveToFile(const std::string& filename) const {
		std::ofstream file(Utilities::GetOutputLocation(filename));
		if (!file.is_open()) {
			throw DynaPlex::Error("Failed to open file for writing: " + filename);
		}
		file << pImpl->data.dump(4);
		file.close();
	}

	VarGroup VarGroup::LoadFromFile(const std::string& filename) {
		auto loc = DynaPlex::Utilities::GetOutputLocation(filename);
		std::ifstream file(loc);
		if (file.is_open()) {
			ordered_json j;
			try {
				file >> j;
			}
			catch (const nlohmann::json::parse_error& e) {
				throw DynaPlex::Error("Failed to parse JSON file: " + filename + " - " + e.what());
			}
			file.close();

			// Check if the loaded JSON adheres to the homogeneity rule for arrays
			try {
				DynaPlex::VarGroupHelpers::check_validity(j);
			}
			catch (const DynaPlex::Error& e)
			{
				throw DynaPlex::Error(std::string("Error in loaded JSON data from ") + loc + ":\n  " + e.what());
			}
			VarGroup VarGroup;
			VarGroup.pImpl->data = std::move(j);
			return VarGroup;
		}
		else {
			throw DynaPlex::Error("Unable to open file for reading: " + filename);
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
