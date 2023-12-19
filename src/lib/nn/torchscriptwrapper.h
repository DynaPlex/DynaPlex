#pragma once
#if DP_TORCH_AVAILABLE	
#include <torch/script.h>
#include <torch/torch.h>
#include <torch/nn.h>
#include "dynaplex/error.h"
namespace DynaPlex::NN {
class TorchScriptWrapper : public torch::nn::Module {
private:
    torch::jit::script::Module script_module_;

public:
    TorchScriptWrapper(const std::string& path_to_scripted_module) {
        // Load the TorchScript module
        script_module_ = torch::jit::load(path_to_scripted_module);
    }

    torch::Tensor forward(torch::Tensor x) {
        // Use the TorchScript module for inference
        return script_module_.forward({ x }).toTensor();
    }
      
};

class TorchScriptDictWrapper : public torch::nn::Module {
private:
    torch::jit::script::Module script_module_;

public:
    TorchScriptDictWrapper(const std::string& path_to_scripted_module) {
        // Load the TorchScript module
        script_module_ = torch::jit::load(path_to_scripted_module);
    }

    torch::Tensor forward(torch::Dict<std::string, torch::Tensor> dict) {
        // Use the TorchScript module for inference
        return script_module_.forward({ dict }).toTensor();
    }

};

}
#endif