#pragma once

#include <string>
#include "dynaplex/vargroup.h"

class NestedClass {
public:
    NestedClass(DynaPlex::VarGroup& vars);
    NestedClass() {}
    void Print() const;

private:
    std::string Id;
    double Size;
};

