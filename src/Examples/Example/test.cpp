#include "MDP_Implementation.h"
#include "dynaplex/utilities.h"
#include "dynaplex2/NNTrainer.h"
#include "dynaplex/LibraryComponent.h"
#include <iostream>
int main()
{
	MDP_Implementation mdp_impl(2);

	//auto mdp2= Dynaplex::Convert( )
	auto mdp = DynaPlex::Convert(mdp_impl);

	DynaPlex::NNTrainer trainer(mdp);
	DynaPlex::LibraryComponent comp{mdp};

	comp.writeidentifier();
	return 0;
}
