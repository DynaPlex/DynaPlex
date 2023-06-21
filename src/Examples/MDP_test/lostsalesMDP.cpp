#include <iostream>
#include "lostsalesMDP.h"
#include <dynaplex/errors.h>

std::string lostsalesMDP::Identifier()
{

	return "implementation "+ std::to_string(j);
}

lostsalesMDP::MDP_Implementation(int j):j{j}
{
}
