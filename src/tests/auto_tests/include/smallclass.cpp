#include "smallclass.h"
#include <iostream>
#include "dynaplex/vargroup.h"
SmallClass::SmallClass(const DynaPlex::VarGroup& vars)
{
    vars.Get("Id", Id);
    vars.Get("Size", Size);
}

DynaPlex::VarGroup SmallClass::ToVarGroup() const
{
    DynaPlex::VarGroup vars;
    vars.Add("Id", Id);
    vars.Add("Size", Size);
    return vars;
}


void SmallClass::Print() const {
    std::cout << "Id: " << Id << std::endl;
    std::cout << "Size: " << Size << std::endl;
}