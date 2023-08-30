#include "SomeClass.h"
#include <iostream>




SomeClass::SomeClass(const DynaPlex::VarGroup& vars)
{
    vars.Get("testEnumClass", testEnumClass);
    vars.Get("myString", myString);
    vars.Get("myInt", myInt);
    vars.Get("myVector", myVector);
    vars.Get("nestedClass", nestedClass);
    vars.Get("myNestedVector", myNestedVector);
    static_assert(DynaPlex::Concepts::AppendableBasicContainer<DynaPlex::Queue<int64_t>>," Queue is not appendable");

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

    static_assert(DynaPlex::Concepts::ReadableBasicContainer<DynaPlex::Queue<int64_t>>, "");


    vars.Add("myQueue", myQueue);
    return vars;
}
