#pragma once

#include <string>
#include <vector>
#include "smallclass.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/queue.h"
class SomeClass {
public:
    explicit SomeClass(const DynaPlex::VarGroup& vars);    
    DynaPlex::VarGroup ToVarGroup() const;



public:
    enum class Test {option = 3,decline = 5};
    Test testEnumClass;
    std::string myString;
    int64_t myInt;
    std::vector<int64_t> myVector;
    DynaPlex::Queue<int64_t> myQueue;
    
    SmallClass nestedClass;
    DynaPlex::Queue<SmallClass> myNestedVector;
};

