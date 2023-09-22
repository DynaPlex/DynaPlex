#include "someclass.h"
#include <iostream>




SomeClass::SomeClass(const DynaPlex::VarGroup& vars)
{
    vars.Get("testEnumClass", testEnumClass);
    vars.Get("myString", myString);
    vars.Get("myInt", myInt);
    vars.Get("myVector", myVector);
    vars.Get("nestedClass", nestedClass);
    vars.Get("myNestedVector", myNestedVector);
   
    vars.Get("myQueue", myQueue);
}

DynaPlex::VarGroup SomeClass::ToVarGroup() const
{
    DynaPlex::VarGroup vars;
    vars.Add("testEnumClass", testEnumClass);
    vars.Add("myString", myString);
    vars.Add("myInt", myInt);
    vars.Add("myVector", myVector);
    vars.Add("nestedClass", nestedClass);
    vars.Add("myNestedVector", myNestedVector);

    

    vars.Add("myQueue", myQueue);
    return vars;
}
