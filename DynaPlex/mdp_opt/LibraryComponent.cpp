#include "LibraryComponent.h"

void LibraryComponent::write()
{
	mdp->write();
	mdp->write();
}

LibraryComponent::LibraryComponent(DynaPlex::MDP mdp):
mdp{mdp}
{}