#include "dynaplex/neuralnetworktrainer.h"
#include <iostream>
#if Torch_available
#include <torch/torch.h>
#endif

namespace DynaPlex {
	void NeuralNetworkTrainer::writeidentifier()
	{
#if Torch_available
		std::cout << "DynaPlex: Pytorch available" << std::endl;
		torch::Tensor eye = torch::eye(2);
		std::cout << eye << std::endl;
		std::cout << torch::cuda::is_available() << std::endl;
#else
		std::cout << "torch not available" << std::endl;
#endif
		if (mdp)
		{
			std::cout << mdp->Identifier() << std::endl;
		}
	}


	NeuralNetworkTrainer::NeuralNetworkTrainer(DynaPlex::MDP mdp) :
		mdp{ mdp }
	{}
}