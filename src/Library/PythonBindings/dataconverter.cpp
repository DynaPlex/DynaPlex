#include "dataconverter.h"

DynaPlex::Params DynaPlex::Converter::ToDynaPlexParams(py::kwargs& kwargs)
{
    auto params = DynaPlex::Params{};

    params.Add("test", 1223);

    return params;
}
