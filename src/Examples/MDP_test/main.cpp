#include "lostsalesMDP.h"
#include "dynaplex/convert.h"
#include "dynaplex/neuralnetworktrainer.h"
#include "dynaplex/librarycomponent.h"
#include "dynaplex/errors.h"
#include <iostream>

int main()
{
	try
	{
		DynaPlex::Params pars({{"p",1}});
		lostsalesMDP mdp_impl(pars);

	}
	catch (const std::exception& e) {
		std::cerr << "Caught exception: " << e.what() << '\n';
	}
	catch (...) {
		std::cerr << "Caught an unknown exception\n";
	}

	return 0;
}
