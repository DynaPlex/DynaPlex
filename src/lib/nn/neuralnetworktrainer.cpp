#include "dynaplex/neuralnetworktrainer.h"
#include <iostream>
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#endif

namespace DynaPlex {
	void NeuralNetworkTrainer::writeidentifier()
	{
#if DP_TORCH_AVAILABLE
		std::cout << "DynaPlex: Pytorch available, Version ";
		std::cout << TORCH_VERSION_MAJOR << ".";
		std::cout << TORCH_VERSION_MINOR << ".";
		std::cout << TORCH_VERSION_PATCH << std::endl;
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