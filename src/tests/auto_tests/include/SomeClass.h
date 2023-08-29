#ifndef SOMECLASS_H
#define SOMECLASS_H

#include <string>
#include <vector>
#include "nestedclass.h"
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
    DynaPlex::Modelling::Queue<int64_t> myQueue;
    
    NestedClass nestedClass;
    DynaPlex::Modelling::Queue<NestedClass> myNestedVector;
    SomeClass();
};

#endif  // SOMECLASS_H
