#include "dynaplex/utilities.h"
#include "dynaplex/VarGroup.h"
#include "nlohmann/json.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "dynaplex/error.h"
#include "picosha2.h"
#if DP_PYBIND_SUPPORT
#include "pybind11/pybind11.h"
#include "pybind11_json/pybind11_json.h"
#endif

namespace DynaPlex {

    using ordered_json = nlohmann::ordered_json;


    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";


    bool check_validity(const ordered_json& j);
 
   

    std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

           // while ((i++ < 3))
            //    ret += '=';
        }

        return ret;
    }


    std::string hash_string(const std::string& str) {
        std::vector<unsigned char> hash(picosha2::k_digest_size);
        picosha2::hash256(str.begin(), str.end(), hash.begin(), hash.end());
        return base64_encode(hash.data(), hash.size());
    }  


    // Function to sort an ordered_json object
    ordered_json sort_json(const ordered_json& j) {
        ordered_json ordered_j;

        // Check if the input is an object
        if (j.is_object()) {
            std::map<std::string, ordered_json> sorted_map;

            // Sort the keys of the object
            for (auto& element : j.items()) {
                sorted_map[element.key()] = element.value();
            }

            // Recursively sort object values
            for (auto& element : sorted_map) {
                ordered_j[element.first] = sort_json(element.second);
            }
        }
        // Check if the input is an array
        else if (j.is_array()) {
            for (auto& element : j) {
                ordered_j.push_back(sort_json(element));
            }
        }
        // If the input is not an object or array, simply assign its value
        else {
            ordered_j = j;
        }

        return ordered_j;
    }



    // Function to hash a sorted ordered_json object
    std::string hash_json(const ordered_json& j) {
        // Sort the JSON object
        ordered_json sorted_j = sort_json(j);

        // Serialize the sorted object to a string
        std::string serialized = sorted_j.dump();

        return hash_string(serialized);
    }

 

    class VarGroup::Impl {
    public:

    
        ordered_json data;

        void PrintAbbrv(ordered_json& obj, std::ostream& os, int indent = 0) {
            const std::string indentStr(indent, ' ');

            if (obj.is_object()) {
                os << "{\n";
                for (auto it = obj.begin(); it != obj.end();) {
                    os << indentStr << "    \"" << it.key() << "\": ";
                    PrintAbbrv(it.value(), os, indent + 4);
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
                    PrintAbbrv(obj[0], os,indent+4);
                    os << ", ";
                    PrintAbbrv(obj[1], os, indent + 4);
                    os << ", ... (" << obj.size() - 3 << " omitted) ..., ";
                    PrintAbbrv(obj[obj.size() - 1], os,indent+4);
                }
                else {
                    for (auto it = obj.begin(); it != obj.end();) {
                        PrintAbbrv(*it, os, indent + 4);
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
        }

        void Add(const std::string& key, const DataType& value) {
             std::visit(
                [this, &key](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, int64_t> || std::is_same_v<T, double>|| std::is_same_v<T, std::nullptr_t>|| std::is_same_v<T, bool>) {
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
        void Get_IntoHelper(const std::string& key, T& out_val) const {
            if (!data.contains(key)) {
                throw DynaPlex::Error("Key \"" + key + "\" not found in VarGroup.");
            }

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
            if (!data.contains(key)) {
                throw DynaPlex::Error("Key \"" + key + "\" not found in VarGroup.");
            }

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
                DynaPlex::check_validity(json);
            }
            catch (const DynaPlex::Error& e)
            {
                throw DynaPlex::Error(std::string("Error in dictionary/kwargs passed from python:\n") + e.what());
            }
            catch (const std::exception& e)
            {
                throw DynaPlex::Error("Error while converting from pybind11::dict to VarGroup");
            }
            VarGroup varGroup{};
            varGroup.pImpl->data = json;
            return varGroup;
        }
    };

    VarGroup::VarGroup() : pImpl(std::make_unique<Impl>()) {}


    VarGroup::VarGroup(TupleList list) : VarGroup() {
        for (const auto& [first,second] : list) {
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
    void VarGroup::Add(std::string s,const Int64Vec& vec) {
        pImpl->Add(s, vec);
    }
    void VarGroup::Add(std::string s,const DoubleVec& vec) {
        pImpl->Add(s, vec);
    }
    void VarGroup::Add(std::string s, const StringVec& vec) {
        pImpl->Add(s, vec);
    }
    void VarGroup::Add(std::string s,const VarGroup& vec) {
        pImpl->Add(s, vec);
    }
    void VarGroup::Add(std::string s,const VarGroupVec& vec) {
        pImpl->Add(s, vec);
    }
    void VarGroup::Get_Into(const std::string& key, int64_t& out_val) const {
        pImpl->Get_IntoHelper(key, out_val);
    }
    void VarGroup::Get_Into(const std::string& key, std::string& out_val) const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, int& out_val) const {
        int64_t int64;
        pImpl->Get_IntoHelper(key, int64);

        if (int64 < INT_MIN || int64 > INT_MAX) {
            throw DynaPlex::Error("int64_t value out of range for conversion to int");
        }
        out_val = static_cast<int>(int64);
    }
   
    void VarGroup::Get_Into(const std::string& key, bool& out_val) const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, double& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, VarGroup::Int64Vec& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, VarGroup::StringVec& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, VarGroup::DoubleVec& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, VarGroup::VarGroupVec& out_val)const {
        pImpl->GetVarGroupVec(key, out_val);
    }

    void VarGroup::Get_Into(const std::string& key, std::vector<int>& out_val) const {
        std::vector<int64_t> tmp;
        pImpl->Get_IntoHelper(key, tmp);


        out_val.clear();
        for (const auto& val : tmp) {
            if (val < std::numeric_limits<int>::min() || val > std::numeric_limits<int>::max()) {
                throw DynaPlex::Error("Cannot convert to std::vector<int>. Value " + std::to_string(val) + " for key " + key + " cannot be represented as int.");
            }
            out_val.push_back(static_cast<int>(val));
        }
    }


    void VarGroup::Get_Into(const std::string& key, VarGroup& out_val) const {
        if (!pImpl->data.contains(key)) {
            throw std::runtime_error("key " + key + " not found in VarGroup.");
        }

        if (!pImpl->data[key].is_object()) {
            throw std::runtime_error("expected object type for key " + key + ", but found " + std::string(pImpl->data[key].type_name()));
        }

        out_val = Impl::ToVarGroup(pImpl->data[key]);
    }

    void VarGroup::Print() const {
        pImpl->PrintAbbrv(pImpl->data, std::cout);
        std::cout << std::endl;       
    }

   

    void VarGroup::SaveToFile(const std::string& filename) const {
        std::ofstream file(Utilities::GetOutputLocation(filename));
        if (!file.is_open()) {
            throw DynaPlex::Error("Failed to open file for writing: " + filename);
        }
        file << pImpl->data.dump(4); 
        file.close();
    }

    bool check_homogeneity(const ordered_json& j, const std::string& path) {
        if (j.is_array()) {
            // Check if the array is homogeneous
            ordered_json::value_t array_type;
            bool first = true;

            for (const auto& element : j) {
                if (first) {
                    if (element.is_number_integer() || element.is_number_unsigned()) {
                        array_type = ordered_json::value_t::number_integer;
                    }
                    else {
                        array_type = element.type();
                    }

                    if (element.is_boolean())
                    {
                        throw DynaPlex::Error("Boolean value not allowed in array, at path: " + path);
                    }
                    if (element.is_array())
                    {
                        throw DynaPlex::Error("Direct nesting of array inside array not supported, at path: " + path);
                    }
                    first = false;
                }
                else {
                    ordered_json::value_t element_type;
                    if (element.is_number_integer() || element.is_number_unsigned()) {
                        element_type = ordered_json::value_t::number_integer;
                    }
                    else {
                        element_type = element.type();
                    }

                    if (array_type != element_type) {
                        throw DynaPlex::Error("Inhomogeneous array at path: " + path);
                    }
                }
            }

            // Check if the array elements are either primitive or structured
            int i{ 0 };
            for (const auto& element : j) {
                std::string key = "[" + std::to_string((i++)) + "]";   
                std::string child_path = path+key;
                if (!check_homogeneity(element, child_path)) {
                    return false;
                }
            }
        }
        else if (j.is_object()) {
            for (const auto& [key, value] : j.items()) {
                std::string child_path = path.empty() ? key : (path + "." + key);
                if (!check_homogeneity(value, child_path)) {
                    return false;
                }
            }
        }
        else
        {
            if (j.is_binary())
            {
                throw DynaPlex::Error("Binary values not allowed");
            }
            if (j.is_discarded())
            {
                throw DynaPlex::Error("Discarded values not allowed");
            }
        }

        return true;
    }

    bool check_validity(const ordered_json& j)
    {
        if (!j.is_object())
        {
            throw DynaPlex::Error("Root node is not an object/dict.");
        }
        return check_homogeneity(j, "");
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
                check_validity(j);
            }
            catch (const DynaPlex::Error& e)
            {
                throw DynaPlex::Error(std::string("Error in loaded JSON data from ") + loc + ":\n  "+ e.what());
            }
            VarGroup VarGroup;
            VarGroup.pImpl->data = std::move(j);
            return VarGroup;
        }
        else {
            throw DynaPlex::Error("Unable to open file for reading: " + filename);
        }
    }


  

    std::string VarGroup::Hash()
    {
        return hash_json(pImpl->data);
    }
        
    VarGroup::VarGroup(const std::string& rawJson)
        : pImpl(std::make_unique<Impl>())
    {
        try {
            pImpl->data = nlohmann::ordered_json::parse(rawJson);
        }
        catch (const nlohmann::json::exception& ex) {
            // Handle the exception appropriately, e.g., throw a custom exception or initialize with an empty JSON
            throw std::runtime_error("Failed to parse JSON string: " + std::string(ex.what()));
        }
    }

    bool VarGroup::operator==(const VarGroup& other) const {
        return pImpl->data == other.pImpl->data;
    }

    bool VarGroup::operator!=(const VarGroup& other) const {
        return !(this->operator==(other));
    }

 


#if DP_PYBIND_SUPPORT
    //We return a unique pointer here (instead of pybind11::dict outright), so that the header does not need to include pybind11 (and can use forward declare).
    // This to avoid longer compilation times as VarGroup.h is needed in many places.  
	std::unique_ptr<pybind11::dict> VarGroup::toPybind11Dict() const
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


	VarGroup::VarGroup(const pybind11::object& obj):pImpl(std::make_unique<Impl>())
	{
        auto json = ConvertToJson(obj);
        // Check if the loaded JSON adheres to the homogeneity rule for arrays
        try {
            check_validity(json);
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