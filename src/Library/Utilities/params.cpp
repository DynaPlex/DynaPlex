#include "dynaplex/utilities.h"
#include "dynaplex/Params.h"
#include "json.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "dynaplex/utilities.h"
#include "dynaplex/errors.h"

using ordered_json = nlohmann::ordered_json;

namespace DynaPlex {


    
    class Params::Impl {
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
                    else if constexpr (std::is_same_v<T, Params>) {
                        data[key] = v.pImpl->data;
                    }
                    else if constexpr (std::is_same_v<T, Int64Vec> || std::is_same_v<T, DoubleVec> || std::is_same_v<T, StringVec>) {
                        data[key] = ordered_json(v);
                    }
                    else if constexpr (std::is_same_v<T, ParamsVec>) {
                        ordered_json jsonArray;
                        for (const auto& p : v) {
                            jsonArray.push_back(p.pImpl->data);
                        }
                        data[key] = jsonArray;
                    }
                    else
                    {
                        throw DynaPlex::Error("Unhandled case in Params implementation.");
                    }
                },
                value);
        }

        template<typename T>
        void Get_IntoHelper(const std::string& key, T& out_val) const {
            if (!data.contains(key)) {
                throw DynaPlex::Error("Key \"" + key + "\" not found in Params.");
            }

            if (!data[key].is_null()) {
                try {
                    out_val = data[key].get<T>();
                }
                catch (nlohmann::json::exception& e) {
                    throw DynaPlex::Error("Key " + key + " is not of the correct type in Params. Error: " + e.what());
                }
            }
            else {
                out_val = T{};
            }
        }

        void GetParamsVec(const std::string& key, Params::ParamsVec& out_val) {
            if (!data.contains(key)) {
                throw DynaPlex::Error("Key \"" + key + "\" not found in Params.");
            }

            if (data[key].is_array()) {
                try {
                    for (const auto& item : data[key]) {
                        if (item.is_object()) {
                            Params p(item);
                            out_val.push_back(p);
                        }
                        else {
                            throw DynaPlex::Error("Key " + key + " is not of the correct type in Params.");
                        }
                    }
                }
                catch (nlohmann::json::exception& e) {
                    throw DynaPlex::Error("Key " + key + " is not of the correct type in Params. Error: " + e.what());
                }
            }
            else {
                throw DynaPlex::Error("Key " + key + " must be of type array in Params.");
            }
        }
    };

    Params::Params() : pImpl(std::make_unique<Impl>()) {}


    Params::Params(TupleList list) : Params() {
        for (const auto& [first,second] : list) {
           pImpl->Add(first, second);
       }
    }


    Params::Params(const Params& other) : pImpl(std::make_unique<Impl>(*other.pImpl)) {}

    Params& Params::operator=(const Params& other) {
        if (this != &other) {
            pImpl = std::make_unique<Impl>(*other.pImpl);
        }
        return *this;
    }

    Params::~Params() = default;

    Params::Params(Params&& other) noexcept
        : pImpl(std::move(other.pImpl)) {}

    Params& Params::operator=(Params&& other) noexcept {
        if (this != &other) {
            pImpl = std::move(other.pImpl);
        }
        return *this;
    }

   

    void Params::Add(std::string s, int val) {
        pImpl->Add(s, static_cast<int64_t>(val));
    }

    void Params::Add(std::string s, int64_t val) {
        pImpl->Add(s, val);
    }
    void Params::Add(std::string s, bool val) {
        pImpl->Add(s, val);
    }


    void Params::Add(std::string s, std::string val) {
        pImpl->Add(s, val);
    }
    void Params::Add(std::string s, double val) {
        pImpl->Add(s, val);
    }
    void Params::Add(std::string s, const std::vector<int>& vec) {
        std::vector<int64_t> vec64(vec.begin(), vec.end());
        pImpl->Add(s, vec64);
    }
    void Params::Add(std::string s,const Int64Vec& vec) {
        pImpl->Add(s, vec);
    }
    void Params::Add(std::string s,const DoubleVec& vec) {
        pImpl->Add(s, vec);
    }
    void Params::Add(std::string s, const StringVec& vec) {
        pImpl->Add(s, vec);
    }
    void Params::Add(std::string s,const Params& vec) {
        pImpl->Add(s, vec);
    }
    void Params::Add(std::string s,const ParamsVec& vec) {
        pImpl->Add(s, vec);
    }
    void Params::Get_Into(const std::string& key, int64_t& out_val) const {
        pImpl->Get_IntoHelper(key, out_val);
    }
    void Params::Get_Into(const std::string& key, std::string& out_val) const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void Params::Get_Into(const std::string& key, int& out_val) const {
        int64_t int64;
        pImpl->Get_IntoHelper(key, int64);

        if (int64 < INT_MIN || int64 > INT_MAX) {
            throw DynaPlex::Error("int64_t value out of range for conversion to int");
        }
        out_val = static_cast<int>(int64);
    }
   
    void Params::Get_Into(const std::string& key, bool& out_val) const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void Params::Get_Into(const std::string& key, double& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void Params::Get_Into(const std::string& key, Params::Int64Vec& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void Params::Get_Into(const std::string& key, Params::StringVec& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void Params::Get_Into(const std::string& key, Params::DoubleVec& out_val)const {
        pImpl->Get_IntoHelper(key, out_val);
    }

    void Params::Get_Into(const std::string& key, Params::ParamsVec& out_val)const {
        pImpl->GetParamsVec(key, out_val);
    }

    void Params::Get_Into(const std::string& key, std::vector<int>& out_val) const {
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


    void Params::Get_Into(const std::string& key, Params& params) const {
        if (!pImpl->data.contains(key)) {
            throw std::runtime_error("key " + key + " not found in params.");
        }

        if (!pImpl->data[key].is_object()) {
            throw std::runtime_error("expected object type for key " + key + ", but found " + std::string(pImpl->data[key].type_name()));
        }

        params = Params(pImpl->data[key]);
    }

    void Params::Print() const {
        pImpl->PrintAbbrv(pImpl->data, std::cout);
        std::cout << std::endl;       
    }

    void Params::SaveToFile(const std::string& filename) const {
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

    Params Params::LoadFromFile(const std::string& filename) {
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
            Params params;
            params.pImpl->data = std::move(j);
            return params;
        }
        else {
            throw DynaPlex::Error("Unable to open file for reading: " + filename);
        }
    }


    Params::Params(ordered_json json) : pImpl(std::make_unique<Impl>()) {
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
            throw DynaPlex::Error("Error while converting from pybind11::dict to Params");
        }
        pImpl->data = json;     
    }

    ordered_json Params::ToJson() const
    {
        return pImpl->data;
    }
        



}  // namespace DynaPlex
