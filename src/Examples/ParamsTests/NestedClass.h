#ifndef NESTEDCLASS_H
#define NESTEDCLASS_H

#include <string>
#include "dynaplex/params.h"

class NestedClass {
public:
    NestedClass(DynaPlex::Params& params);
    NestedClass() {}
    void Print() const;

private:
    std::string Id;
    double Size;
};

#endif  // NESTEDCLASS_H
