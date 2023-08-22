#include "NestedClass.h"
#include <iostream>
#include "dynaplex/vargroup.h"
NestedClass::NestedClass(const DynaPlex::VarGroup& vars)
{
    vars.Get("Id", Id);
    vars.Get("Size", Size);
}

DynaPlex::VarGroup NestedClass::ToVarGroup() const
{
    DynaPlex::VarGroup vars;
    vars.Add("Id", Id);
    vars.Add("Size", Size);
    return vars;
}


void NestedClass::Print() const {
    std::cout << "Id: " << Id << std::endl;
    std::cout << "Size: " << Size << std::endl;
}