#include "SomeClass.h"
#include <iostream>

SomeClass::SomeClass(DynaPlex::VarGroup& vars) {
    vars.Get("testEnumClass", testEnumClass);
    vars.Get("myString", myString);
    vars.Get("myInt", myInt);
    vars.Get("myVector", myVector);
    vars.Get("nestedClass", nestedClass);
    vars.Get("myNestedVector", myNestedVector);

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