#include <iostream>
#include "MDP_Implementation.h"


std::string MDP_Implementation::Identifier()
{

	return "implementation "+ std::to_string(j);
}

MDP_Implementation::MDP_Implementation(int j):j{j}
{
}


