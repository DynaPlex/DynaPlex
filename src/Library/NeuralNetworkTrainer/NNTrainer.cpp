#include "dynaplex2/NNTrainer.h"
#include <iostream>
//#include <torch/torch.h>


namespace DynaPlex {
	void NNTrainer::writeidentifier()
	{
	//	torch::Tensor eye = torch::eye(2);
		//std::cout << eye << std::endl;
		std::cout << mdp->Identifier() << std::endl;
	}

	NNTrainer::NNTrainer(DynaPlex::MDP mdp) :
		mdp{ mdp }
	{}
}