#include "dynaplex/utilities.h"
#include "dynaplex/Params.h"
#include "json.hpp"
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
                    PrintAbbrv(obj[0], os);
                    os << ", ... (" << obj.size() - 2 << " omitted) ..., ";
                    PrintAbbrv(obj[obj.size() - 1], os);
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
                    else if constexpr (std::is_same_v<T, IntVec> || std::is_same_v<T, DoubleVec>) {
                        data[key] = ordered_json(v);
                    }
                    else if constexpr (std::is_same_v<T, ParamsVec>) {
                        ordered_json jsonArray;
                        for (const auto& p : v) {
                            jsonArray.push_back(p.pImpl->data);
                        }
                        data[key] = jsonArray;
                    }
                },
                value);
        }
    };

    Params::Params() : pImpl(std::make_shared<Impl>()) {}

    Params::Params(TupleList list) : Params() {
        for (const auto& t : list) {
           pImpl->Add(std::get<0>(t), std::get<1>(t));
       }
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

    void Params::Add(std::string s, IntVec vec) {
        pImpl->Add(s, vec);
    }

    void Params::Add(std::string s, DoubleVec vec) {
        pImpl->Add(s, vec);
    }

    void Params::Add(std::string s, Params vec) {
        pImpl->Add(s, vec);
    }


    void Params::Add(std::string s, ParamsVec vec) {
        pImpl->Add(s, vec);
    }

    void Params::Print() {
        pImpl->PrintAbbrv(pImpl->data, std::cout);
        std::cout << std::endl;
        //Classic dump - not always very readable. 
        // std::cout << pImpl->data.dump(4) << std::endl;
    }

    void Params::SaveToFile(const std::string& filename) const {
        std::ofstream file(Utilities::GetOutputLocation(filename));
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }
        file << pImpl->data.dump(4); // You can change the number '4' to adjust the indentation in the output file
        file.close();
    }

    bool is_supported_type(const ordered_json& element, bool inside_array, const std::string& path);

    bool is_homogeneous_array(const ordered_json& j, const std::string& path) {
        if (!j.is_array() || j.empty()) {
            return true;
        }

        const auto& first_element = *j.begin();
        if (!is_supported_type(first_element, true, path)) {
            return false;
        }

        for (const auto& element : j) {
            if (first_element.type() != element.type() || !is_supported_type(element, true, path)) {
                return false;
            }
        }
        return true;
    }

    bool is_supported_type(const ordered_json& element, bool inside_array, const std::string& path) {
        if (element.is_number_integer() || element.is_number_float() ||
            (!inside_array && (element.is_string() ||element.is_null() || element.is_boolean() ))) {
            return true;
        }
        else if (element.is_object()) {
            for (const auto& [key,value] : element.items()) {
                if (!is_supported_type(value, false, path + "." + key)) {
                    return false;
                }
            }
            return true;
        }
        else if (element.is_array()) {
            return is_homogeneous_array(element, path);
        }
        throw DynaPlex::Error("Invalid type found in JSON at path: " + path);
    }



    void check_if_valid(const ordered_json& j, const std::string& path) {
        for (const auto& item : j.items()) {
            const auto& key = item.key();
            const auto& value = item.value();
            if (value.is_array()) {
                if (!is_homogeneous_array(value, path + "." + key)) {
                    throw DynaPlex::Error("Invalid or heterogeneous array found in JSON at path: " + path + "." + key);
                }
            }
            else if (value.is_object()) {
                check_if_valid(value, path + "." + key);
            }
            else if (!is_supported_type(value, false, path + "." + key)) {
                throw DynaPlex::Error("Invalid type found in JSON at path: " + path + "." + key);
            }
        }
    }

    Params Params::LoadFromFile(const std::string& filename) {
        std::ifstream file(DynaPlex::Utilities::GetOutputLocation( filename));
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
            check_if_valid(j, "");

            Params params;
            params.pImpl->data = std::move(j);
            return params;
        }
        else {
            throw DynaPlex::Error("Unable to open file for reading: " + filename);
        }
    }


}  // namespace DynaPlex
