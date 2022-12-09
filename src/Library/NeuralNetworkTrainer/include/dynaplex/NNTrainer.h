#pragma once
#include "dynaplex/MDP.h"

namespace DynaPlex {
	class NNTrainer
	{

		DynaPlex::MDP mdp;
	public:
		void writeidentifier();


		NNTrainer(DynaPlex::MDP mdp);
	};
}//namespace DynaPlex