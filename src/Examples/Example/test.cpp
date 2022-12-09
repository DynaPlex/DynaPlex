#include "mdp_implementation.h"
#include "dynaplex/utilities.h"
#include "dynaplex/neuralnetworktrainer.h"
#include "dynaplex/librarycomponent.h"
#include <iostream>
int main()
{
	MDP_Implementation mdp_impl(2);

	auto mdp = DynaPlex::Convert(mdp_impl);

	DynaPlex::NeuralNetworkTrainer trainer(mdp);
	DynaPlex::LibraryComponent comp{mdp};

	trainer.writeidentifier();
	return 0;
}
