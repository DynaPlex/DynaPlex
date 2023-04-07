#include "dynaplex/utilities.h"
#include <iostream>

void DynaPlex::Utilities::Fail(std::string message)
{
	std::cout << message << std::endl;
	throw std::runtime_error(message);
}
