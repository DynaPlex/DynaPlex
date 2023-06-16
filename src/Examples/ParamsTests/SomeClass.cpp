#include "SomeClass.h"
#include <iostream>

SomeClass::SomeClass(const DynaPlex::Params& params) {
    
    params.GetValue("myString", myString);
    params.GetValue("myInt", myInt);
    params.GetValue("myVector", myVector);

   //params.GetValue("nestedClass", nestedClass);

}

void SomeClass::Print() const {
    std::cout << "myString: " << myString << std::endl;
    std::cout << "myInt: " << myInt << std::endl;
    std::cout << "myVector: ";
    for (const auto& num : myVector) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    std::cout << "myNestedVector: " << std::endl;
    for (const auto& nestedObj : myNestedVector) {
        nestedObj.Print();
        std::cout << std::endl;
    }
}