#include "NestedClass.h"
#include "NestedClass.h"
#include <iostream>

NestedClass::NestedClass(DynaPlex::Params& params)
{
    params.GetInto("Id", Id);
    params.GetInto("Size", Size);
}


void NestedClass::Print() const {
    std::cout << "Id: " << Id << std::endl;
    std::cout << "Size: " << Size << std::endl;
}