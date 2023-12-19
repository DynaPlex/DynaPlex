#include "dynaplex/trainedpolicyprovider.h"
#include "neuralnetworkprovider.h"
#include "torchscriptwrapper.h"
#include "nn_policy.h"
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#endif
//#if DP_TORCH_AVAILABLE
namespace DynaPlex {


	DynaPlex::Policy TrainedPolicyProvider::LoadPolicy(DynaPlex::MDP mdp, std::string path_to_policy_without_extension)
	{
		//policy is saved over two different files, architecture (json) and weights (pth). 
		auto path_to_json = System::SetFileExtension(path_to_policy_without_extension, "json");
		auto path_to_weights = System::SetFileExtension(path_to_policy_without_extension, "pth");
		
		//load architecture:
		auto policy_config = VarGroup::LoadFromFile(path_to_json);
		std::string id;
			
		policy_config.Get("id", id);
#if DP_TORCH_AVAILABLE		
		if (id == "NN_Policy")
		{
			//check whether dimensionalities are matching:
			int64_t num_inputs, num_outputs;
			policy_config.Get("num_inputs", num_inputs);
			if (num_inputs != mdp->NumFlatFeatures())
				throw DynaPlex::Error("NeuralNetworkProvider::LoadPolicy - cannot create neural network policy from loaded data for this mdp because num_inputs for loaded policy does not match mdp->NumFlatFeatures().");
			policy_config.Get("num_outputs", num_outputs);
			if (num_outputs != mdp->NumValidActions())
				throw DynaPlex::Error("NeuralNetworkProvider::LoadPolicy - cannot create neural network policy from loaded data for this mdp because num_outputs for the loaded policy does not match mdp->NumValidActions(). ");
	
			//create policy:
			auto policy = std::make_shared<NN_Policy>(mdp);
			DynaPlex::VarGroup nn_architecture;
			policy_config.Get("nn_architecture", nn_architecture);
			NeuralNetworkProvider provider(mdp);
			policy->neural_network = std::make_unique<torch::nn::AnyModule>(provider.GetTrainableNN(nn_architecture));
			//loading weights:
			auto as_nn_module = policy->neural_network->ptr();
			torch::load(as_nn_module, path_to_weights);
			//set config:
			policy->policy_config = policy_config;
			return policy;

		}
		else if (id == "torchscript")
		{

			auto policy = std::make_shared<NN_Policy>(mdp);
			// Use the TorchScriptWrapper class to wrap the TorchScript module
			std::string input_type;
			if (policy_config.HasKey("input_type"))
			{
				policy_config.Get("input_type", input_type);
			}
			else
			{			
				std::cout << "input_type not provided in json for torchscripted file. Note that torchscript .json files without input_type is deprecated. Set to \"tensor\" for torchscripts that expect tensor in forward function. ";
				input_type = "tensor";
			}
			
			if (input_type == "tensor")
			{
				auto torchscript_wrapper = std::make_shared<DynaPlex::NN::TorchScriptWrapper>(path_to_weights);
				policy->neural_network = std::make_unique<torch::nn::AnyModule>(torchscript_wrapper);
				policy->fw_type = NN_Policy::NetworkForwardType::Tensor;

			}
			else if (input_type == "dict" || input_type == "dict_with_mask")
			{
				auto torchscript_wrapper = std::make_shared<DynaPlex::NN::TorchScriptDictWrapper>(path_to_weights);
				policy->neural_network = std::make_unique<torch::nn::AnyModule>(torchscript_wrapper);
				if (input_type == "dict")
					policy->fw_type = NN_Policy::NetworkForwardType::TensorDict;
				else
					policy->fw_type = NN_Policy::NetworkForwardType::TensorDictMask;
			}
			else
			{
				throw DynaPlex::Error("NeuralNetworkProvider::LoadPolicy: input_type of neural network is: " + input_type + ". This input_type is not available.");

			}
			policy->policy_config = policy_config;
			return policy;
		}
		else
		{
			throw DynaPlex::Error("NeuralNetworkProvider::LoadPolicy: id of neural network is: "+ id + ". This id is not available.");
		}
#else
		throw DynaPlex::Error("NeuralNetworkProvider::LoadPolicy: Torch not available - Cannot construct. To make torch available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt ");
#endif
	}
	void TrainedPolicyProvider::SavePolicy(DynaPlex::Policy policy, std::string path_to_policy_without_extension)
	{
		std::string id;
		auto policy_config = policy->GetConfig();
		policy_config.Get("id", id);

		if (id == "NN_Policy")
		{
			std::shared_ptr<NN_Policy> as_NN_policy = std::dynamic_pointer_cast<NN_Policy>(policy);
			
			if (!as_NN_policy) {
				throw DynaPlex::Error("NeuralNetworkProvider::SavePolicy - cannot save this policy of declared type+ " + id + ". Cast to NN_Policy fails.");
			}
#if DP_TORCH_AVAILABLE		
			auto weights_path = System::SetFileExtension(path_to_policy_without_extension, "pth");
			torch::save(as_NN_policy->neural_network->ptr(), weights_path);
			auto json_path = System::SetFileExtension(path_to_policy_without_extension, "json");
			as_NN_policy->policy_config.SaveToFile(json_path, 1);
#else
			throw DynaPlex::Error("NeuralNetworkProvider::SavePolicy - Torch not available, cannot save. To make torch available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt ");

#endif
		}
		else {
			throw DynaPlex::Error("NeuralNetworkProvider::SavePolicy - do not know how to save policy of declared type+ " + id + ".");
		}
	}
}//namespace DynaPlex