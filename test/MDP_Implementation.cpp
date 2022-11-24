#include <iostream>
#include "MDP_Implementation.h"


void MDP_Implementation::write()
{
	for (size_t i = 0; i < j; i++)
	{
		std::cout << "specific implementation" << std::endl;
	}
}

MDP_Implementation::MDP_Implementation(int j):j{j}
{
}


