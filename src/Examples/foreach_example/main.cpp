#include "mdp_implementation.h"
#include "dynaplex/convert.h"
#include "dynaplex/neuralnetworktrainer.h"
#include "dynaplex/librarycomponent.h"
#include <iostream>
int main()
{

	
	MDP_Implementation mdp_impl(2);
	MDP_Implementation::State state{ 4 };

	auto mdp = DynaPlex::Convert(mdp_impl);
	DynaPlex::NeuralNetworkTrainer trainer(mdp);
	trainer.writeidentifier();

	auto list = mdp_impl.AllowedActions(state);

	for (size_t a : list)
	{
		std::cout << a << std::endl;
	}

	return 0;



	//DynaPlex::LibraryComponent comp{mdp};

	//trainer.writeidentifier();
	return 0;
}
