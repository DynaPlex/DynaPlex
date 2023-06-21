#include "SomeClass.h"
#include <iostream>

SomeClass::SomeClass(DynaPlex::Params& params) {
    
    params.GetInto("myString", myString);
    params.GetInto("myInt", myInt);
    params.GetInto("myVector", myVector);

    params.GetInto("nestedClass", nestedClass);
    params.GetInto("myNestedVector", myNestedVector);

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