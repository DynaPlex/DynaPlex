#include "NestedClass.h"
#include <iostream>
#include "dynaplex/params.h"
NestedClass::NestedClass(DynaPlex::Params& params)
{
    params.Get_Into("Id", Id);
    params.Get_Into("Size", Size);
}


void NestedClass::Print() const {
    std::cout << "Id: " << Id << std::endl;
    std::cout << "Size: " << Size << std::endl;
}