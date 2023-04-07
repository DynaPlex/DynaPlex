#include <pybind11/pybind11.h>
#include <iostream>
#include <variant>
#include "dataconverter.h"

namespace py = pybind11;


enum class Type{py_str,py_int,py_float};

std::variant< std::string, long long, double > processhandle(py::handle& handle)
{
    std::unordered_map<std::string, Type> map{
        {"<class 'str'>",Type::py_str},
        {"<class 'int'>",Type::py_int},
        {"<class 'float'>",Type::py_float}
    };
    std::variant<std::string, long long, double> value;

    std::vector<std::variant<std::string, long long, double>> list;
    list.push_back("true");
    list.push_back(1.0);

    for (auto elem:list)
    {
        auto type = static_cast<Type>(elem.index());

        if (type == Type::py_str)
        {
            std::cout << std::get<std::string>(elem) << std::endl;
        }

    }

    std::string type = py::type::of(handle).str();
    if (map.contains(type))
    {
        switch(map[type])
        {
        case Type::py_str:
            value = handle.cast<std::string>();
            std::cout << "string " << std::get<std::string>(value) << "  " << value.index() << std::endl;
            break;
        case Type::py_int:
            value = handle.cast<long long>();
            std::cout << "int " << std::get<long long>(value) << "  " << value.index() << std::endl;
            break;        
        case Type::py_float:
            value = handle.cast<double>();
            std::cout << "float " << std::get<double>(value) <<"  " << value.index() << std::endl;
            break;       
        }
    }
    else
    {
        throw std::exception("This won't work");
    }
    return value;
}


void process(py::kwargs kwargs)
{
    auto pars = DynaPlex::Converter::ToDynaPlexParams(kwargs);
    std::cout << "---" << std::endl;
    pars.Print();
    std::cout << "---" << std::endl;
    return;
    for (auto kv : kwargs)
    {
        processhandle(kv.first);
        processhandle(kv.second);
    }
}



PYBIND11_MODULE(DynaPlex, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    m.def("process", &process, "Processes kwargs");
}