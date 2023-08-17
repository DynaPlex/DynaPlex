#include "NestedClass.h"
#include <iostream>
#include "dynaplex/vargroup.h"
NestedClass::NestedClass(DynaPlex::VarGroup& vars)
{
    vars.Get_Into("Id", Id);
    vars.Get_Into("Size", Size);
}


void NestedClass::Print() const {
    std::cout << "Id: " << Id << std::endl;
    std::cout << "Size: " << Size << std::endl;
}