#include "dynaplex/librarycomponent.h"
#include <iostream>

namespace DynaPlex {
	void LibraryComponent::writeidentifier()
	{
		std::cout << mdp->Identifier() << std::endl;
	}

	LibraryComponent::LibraryComponent(DynaPlex::MDP mdp) :
		mdp{ mdp }
	{}
}