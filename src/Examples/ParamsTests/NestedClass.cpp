#include "NestedClass.h"
#include "NestedClass.h"
#include <iostream>


NestedClass::NestedClass(DynaPlex::Params& params)
{
    params.GetValue("Id", Id);
    params.GetValue("Size", Size);
}

void NestedClass::Print() const {
    std::cout << "Id: " << Id << std::endl;
    std::cout << "Size: " << Size << std::endl;
}