#pragma once
#include <string>
#include <functional>
#include <iostream>
#include "dynaplex/params.h"

class lostsalesMDP
{
public:
	int j;

	std::string Identifier();

	lostsalesMDP(const DynaPlex::Params& params);
};