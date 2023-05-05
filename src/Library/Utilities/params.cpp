#include "dynaplex/utilities.h"
#include "dynaplex/Params.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "dynaplex/utilities.h"

using json = nlohmann::ordered_json;

namespace DynaPlex {

    class Params::Impl {
    public:
        json data;

        void PrintAbbrv(json& obj, std::ostream& os, int indent = 0) {
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
                    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, long> || std::is_same_v<T, double>) {
                        data[key] = v;
                    }
                    else if constexpr (std::is_same_v<T, Params>) {
                        data[key] = v.pImpl->data;
                    }
                    else if constexpr (std::is_same_v<T, LongVec> || std::is_same_v<T, DoubleVec>) {
                        data[key] = json(v);
                    }
                    else if constexpr (std::is_same_v<T, ParamsVec>) {
                        json jsonArray;
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

    Params& Params::Add(std::string s, int val) {
        pImpl->Add(s, static_cast<long>(val));
        return *this;
    }

    Params& Params::Add(std::string s, long val) {
        pImpl->Add(s, val);
        return *this;
    }

    Params& Params::Add(std::string s, std::string val) {
        pImpl->Add(s, val);
        return *this;
    }

    Params& Params::Add(std::string s, double val) {
        pImpl->Add(s, val);
        return *this;
    }

    Params& Params::Add(std::string s, LongVec vec) {
        pImpl->Add(s, vec);
        return *this;
    }

    Params& Params::Add(std::string s, DoubleVec vec) {
        pImpl->Add(s, vec);
        return *this;
    }

    Params& Params::Add(std::string s, Params vec) {
        pImpl->Add(s, vec);
        return *this;
    }


    Params& Params::Add(std::string s, ParamsVec vec) {
        pImpl->Add(s, vec);
        return *this;
    }

    void Params::Print() {
      //  std::cout << pImpl->data.dump(4) << std::endl;
    }

    void Params::PrintAbbrv() {
        pImpl->PrintAbbrv(pImpl->data, std::cout);
        std::cout << std::endl;
    }

    void Params::SaveToFile(const std::string& filename) const {
        std::ofstream file(Utilities::GetOutputLocation(filename));
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }
        file << pImpl->data.dump(4); // You can change the number '4' to adjust the indentation in the output file
        file.close();
    }

    Params Params::LoadFromFile(const std::string& filename) {
        std::ifstream file(Utilities::GetOutputLocation( filename));
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + filename);
        }
        json data;
        file >> data;
        file.close();

        Params params;
        params.pImpl->data = std::move(data);
        return params;
    }


}  // namespace DynaPlex
