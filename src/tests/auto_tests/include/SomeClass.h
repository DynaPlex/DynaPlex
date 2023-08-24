#ifndef SOMECLASS_H
#define SOMECLASS_H

#include <string>
#include <vector>
#include "nestedclass.h"
#include "dynaplex/vargroup.h"

class SomeClass {
public:
    explicit SomeClass(const DynaPlex::VarGroup& vars);    
    DynaPlex::VarGroup ToVarGroup() const;



    void Print() const;

public:
    enum class Test {option = 3,decline = 5};
    Test testEnumClass;
    std::string myString;
    int myInt;
    std::vector<int> myVector;
    NestedClass nestedClass;
    std::vector<NestedClass> myNestedVector;
    SomeClass();
};

#endif  // SOMECLASS_H
