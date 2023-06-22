#ifndef SOMECLASS_H
#define SOMECLASS_H

#include <string>
#include <vector>
#include "NestedClass.h"
#include "dynaplex/params.h"

class SomeClass {
public:
    SomeClass(DynaPlex::Params& params);

    void Print() const;

private:
    std::string myString;
    int myInt;
    std::vector<int> myVector;
    NestedClass nestedClass;
    std::vector<NestedClass> myNestedVector;
};

#endif  // SOMECLASS_H
