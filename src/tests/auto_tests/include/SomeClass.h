#ifndef SOMECLASS_H
#define SOMECLASS_H

#include <string>
#include <vector>
#include "NestedClass.h"
#include "dynaplex/vargroup.h"

class SomeClass {
public:
    SomeClass(DynaPlex::VarGroup& vars);

    void Print() const;

public:
    std::string myString;
    int myInt;
    std::vector<int> myVector;
    NestedClass nestedClass;
    std::vector<NestedClass> myNestedVector;
};

#endif  // SOMECLASS_H
