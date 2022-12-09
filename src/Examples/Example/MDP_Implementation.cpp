#include <iostream>
#include "mdp_implementation.h"


std::string MDP_Implementation::Identifier()
{

	return "implementation "+ std::to_string(j);
}

MDP_Implementation::MDP_Implementation(int j):j{j}
{
}


