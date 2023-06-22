#pragma once
#include "dynaplex/mdp.h"

namespace DynaPlex {
	class LibraryComponent
	{

		DynaPlex::MDP mdp;
	public:
		void writeidentifier();


		LibraryComponent(DynaPlex::MDP mdp);
	};
}//namespace DynaPlex