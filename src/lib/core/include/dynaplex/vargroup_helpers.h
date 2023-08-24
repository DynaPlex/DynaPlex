#pragma once
#include "dynaplex/vargroup.h"

#define DP_GET_H(vars,element) vars.Get(#element,element)
#define DP_GET(vars,element1) DP_GET_H(vars,element1)
#define DP_GET2(vars,element1,element2) DP_GET_H(vars,element1);DP_GET_H(vars,element2)
#define DP_GET3(vars,element1,element2,element3) DP_GET_H(vars,element1);DP_GET_H(vars,element2);DP_GET_H(vars,element3)