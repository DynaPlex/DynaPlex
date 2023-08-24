#include "SomeClass.h"
#include "dynaplex/vargroup_helpers.h"
#include <iostream>




SomeClass::SomeClass(const DynaPlex::VarGroup& vars)
{
    vars.Get("testEnumClass", testEnumClass);
    vars.Get("myString", myString);
    vars.Get("myInt", myInt);
    vars.Get("myVector", myVector);
    vars.Get("nestedClass", nestedClass);
    vars.Get("myNestedVector", myNestedVector);
}

DynaPlex::VarGroup SomeClass::ToVarGroup() const
{
    DynaPlex::VarGroup vars;
    vars.Add("testEnumClass", testEnumClass);
    vars.Add("myString", myString);
    vars.Add("myInt", myInt);
    vars.Add("myVector", myVector);
    vars.Add("nestedClass", nestedClass);
    vars.Add("myNestedVector", myNestedVector);
    return vars;
}





void SomeClass::Print() const {
    std::cout << "testEnumClass: " << static_cast<int>( testEnumClass) << std::endl;
    std::cout << "myString: " << myString << std::endl;
    std::cout << "myInt: " << myInt << std::endl;
    std::cout << "myVector: ";
    for (const auto& num : myVector) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    std::cout << "nestedClass: " << std::endl;
    nestedClass.Print();

    std::cout << "myNestedVector: " << std::endl;
    for (const auto& nestedObj : myNestedVector) {
        nestedObj.Print();
        std::cout << std::endl;
    }
}