#include "smallclass.h"
#include <iostream>
#include "dynaplex/vargroup.h"
SmallClass::SmallClass(const DynaPlex::VarGroup& vars)
{
    vars.Get("name", name);
    vars.Get("Size", Size);
}

DynaPlex::VarGroup SmallClass::ToVarGroup() const
{
    DynaPlex::VarGroup vars;
    vars.Add("name", name);
    vars.Add("Size", Size);
    return vars;
}


void SmallClass::Print() const {
    std::cout << "name: " << name << std::endl;
    std::cout << "Size: " << Size << std::endl;
}