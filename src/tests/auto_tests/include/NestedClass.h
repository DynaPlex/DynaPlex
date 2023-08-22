#pragma once

#include <string>
#include "dynaplex/vargroup.h"

class NestedClass {
public:
    explicit NestedClass(const DynaPlex::VarGroup& vars);
    DynaPlex::VarGroup ToVarGroup() const;

    NestedClass() {}
    void Print() const;

    std::string Id;
    double Size;
};

