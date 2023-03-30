#include <iostream>
#include "mdp_implementation.h"


std::string MDP_Implementation::Identifier()
{

	return "implementation "+ std::to_string(j);
}

MDP_Implementation::MDP_Implementation(int j):j{j}
{
}

MDP_Implementation::Action MDP_Implementation::ActionTraverser::begin()
{
	size_t initvalue{ 0 };
	while (initvalue < maxvalue)
	{
		if (IsAllowed( ++initvalue))
		{
			return Action(this, initvalue);
		}
	}
	throw;
}
MDP_Implementation::Action MDP_Implementation::ActionTraverser::end()
{
	return Action(this,maxvalue);
}

size_t MDP_Implementation::Action::operator*()
{
	return curvalue;	
}

void MDP_Implementation::Action::operator++()
{	
	while (curvalue < traverser->maxvalue)
	{
		if(traverser->IsAllowed(++curvalue))
		{
			return;
		}
	}
}

bool MDP_Implementation::Action::operator!=(Action action)
{
	return curvalue != action.curvalue;
}

MDP_Implementation::Action::Action(ActionTraverser* traverser, size_t curvalue)
	:
	traverser{ traverser }, curvalue{ curvalue }
{

}