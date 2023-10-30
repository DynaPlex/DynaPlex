#pragma once
#include <string>
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#endif
#include "dynaplex/vargroup.h"
#include "dynaplex/mdp.h"

namespace DynaPlex {	

	class NeuralNetworkProvider {
		DynaPlex::MDP mdp;
	public:
		NeuralNetworkProvider(DynaPlex::MDP mdp);

		#if DP_TORCH_AVAILABLE
		torch::nn::AnyModule GetTrainableNN(DynaPlex::VarGroup nn_config);
		#endif
	};

}//namespace DynaPlex


