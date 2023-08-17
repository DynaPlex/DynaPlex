#include "SomeClass.h"
#include <iostream>

SomeClass::SomeClass(DynaPlex::VarGroup& vars) {
    
    vars.Get_Into("myString", myString);
    vars.Get_Into("myInt", myInt);
    vars.Get_Into("myVector", myVector);

    vars.Get_Into("nestedClass", nestedClass);
    vars.Get_Into("myNestedVector", myNestedVector);

}

void SomeClass::Print() const {
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