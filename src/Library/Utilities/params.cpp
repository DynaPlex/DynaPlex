#include "dynaplex/utilities.h"
#include "dynaplex/Params.h"
#include "json.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "dynaplex/utilities.h"
#include "dynaplex/errors.h"

#if pybind11_support
#include <pybind11/pybind11.h>
#include "pybind11_json.h"
namespace py = pybind11;
#endif

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
                    else if constexpr (std::is_same_v<T, IntVec> || std::is_same_v<T, DoubleVec> || std::is_same_v<T, StringVec> ) {
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
    void Params::Add(std::string s, nullptr_t val) {
        pImpl->Add(s, val);
    }
    void Params::Add(std::string s, std::string val) {
        pImpl->Add(s, val);
    }
    void Params::Add(std::string s, double val) {
        pImpl->Add(s, val);
    }
    void Params::Add(std::string s,const IntVec& vec) {
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

    void Params::Print() {
        pImpl->PrintAbbrv(pImpl->data, std::cout);
        std::cout << std::endl;       
    }

    void Params::SaveToFile(const std::string& filename) const {
        std::ofstream file(Utilities::GetOutputLocation(filename));
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
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
        check_homogeneity(j, "");
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


    Params::Params(pybind11::dict& dict) : pImpl(std::make_unique<Impl>()) {
#if pybind11_support
        // Check if the loaded JSON adheres to the homogeneity rule for arrays
        ordered_json j;
        try {
            j = dict;
            check_validity(j);
        }
        catch (const DynaPlex::Error& e)
        {
            throw DynaPlex::Error(std::string("Error in dictionary/kwargs passed from python:\n") + e.what());
        }
        catch (const std::exception& e)
        {
            throw DynaPlex::Error("Error while converting from pybind11::dict to Params");
        }
        pImpl->data = std::move(j);
#else
        throw DynaPlex::Error("Pybind11 support disabled, cannot convert from pybind11::dict to params");
#endif
    }

}  // namespace DynaPlex
