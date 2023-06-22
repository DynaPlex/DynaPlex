#pragma once
#include "dynaplex/mdp.h"

namespace DynaPlex {
	class NeuralNetworkTrainer
	{

		DynaPlex::MDP mdp;
	public:
		void writeidentifier();

		NeuralNetworkTrainer():mdp{nullptr}
		{};
		NeuralNetworkTrainer(DynaPlex::MDP mdp);
	};
}//namespace DynaPlex