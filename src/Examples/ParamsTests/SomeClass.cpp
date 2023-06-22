#include "SomeClass.h"
#include <iostream>

SomeClass::SomeClass(DynaPlex::Params& params) {
    
    params.Get_Into("myString", myString);
    params.Get_Into("myInt", myInt);
    params.Get_Into("myVector", myVector);

    params.Get_Into("nestedClass", nestedClass);
    params.Get_Into("myNestedVector", myNestedVector);

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