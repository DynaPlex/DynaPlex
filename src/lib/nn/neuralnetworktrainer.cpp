#include "dynaplex/neuralnetworktrainer.h"
#include <iostream>
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#endif

namespace DynaPlex {
    std::string NeuralNetworkTrainer::TorchAvailability()
    {
        std::string result;

#if DP_TORCH_AVAILABLE
        result = "DynaPlex: torch available, Version ";
        result += std::to_string(TORCH_VERSION_MAJOR) + ".";
        result += std::to_string(TORCH_VERSION_MINOR) + ".";
        result += std::to_string(TORCH_VERSION_PATCH) + "\n";
        result += "CUDA AVAILABILITY: " + std::to_string(torch::cuda::is_available()) + "\n";
#else
        result = "torch not available\n";
#endif	

        return result;
    }

	NeuralNetworkTrainer::NeuralNetworkTrainer()
	{}
}