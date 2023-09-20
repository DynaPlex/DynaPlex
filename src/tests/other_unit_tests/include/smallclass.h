#pragma once

#include <string>
#include "dynaplex/vargroup.h"


//Example of a small class that is VarGroupConvertible
class SmallClass {
public:
    explicit SmallClass(const DynaPlex::VarGroup& vars);
    DynaPlex::VarGroup ToVarGroup() const;

    SmallClass() {}
    void Print() const;

    std::string Id;
    double Size;
};

//quick check that the class is convertible
static_assert(DynaPlex::Concepts::VarGroupConvertible<SmallClass>);

