#ifndef SOMECLASS_H
#define SOMECLASS_H

#include <string>
#include <vector>
#include "nestedclass.h"
#include "dynaplex/vargroup.h"

class SomeClass {
public:
    SomeClass(DynaPlex::VarGroup& vars);

    void Print() const;

public:
    enum class Test {option = 3,decline = 5};
    Test testEnumClass;
    std::string myString;
    int myInt;
    std::vector<int> myVector;
    NestedClass nestedClass;
    std::vector<NestedClass> myNestedVector;
};

#endif  // SOMECLASS_H
