#pragma once
#include <string>
#include <functional>
#include <iostream>
#include "dynaplex/vargroup.h"

class lostsalesMDP
{
public:
	int j;

	std::string Identifier();

	lostsalesMDP(const DynaPlex::VarGroup& vars);
};