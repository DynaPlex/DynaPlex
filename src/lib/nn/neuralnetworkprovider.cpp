#include "neuralnetworkprovider.h"
#include "nn_policy.h"
//#if DP_TORCH_AVAILABLE
namespace DynaPlex {


#if DP_TORCH_AVAILABLE
	namespace NeuralNetworks {
		class MLP : public torch::nn::Module {
		private:
			torch::nn::Sequential network;

		public:
			MLP(const DynaPlex::MDP& mdp, const DynaPlex::VarGroup& nn_config) {
				// Extract hidden layers configuration
				std::vector<int64_t> hidden_layers;
				nn_config.Get("hidden_layers", hidden_layers);

				int64_t input_layer = mdp->NumFlatFeatures();
				int64_t output_layer = mdp->NumValidActions();

				// Construct the network
				network = register_module("network", torch::nn::Sequential());

				if (hidden_layers.empty()) {
					// Directly connect input to output if no hidden layers
					network->push_back(torch::nn::Linear(input_layer, output_layer));
				}
				else {
					// Input layer to first hidden layer
					network->push_back(torch::nn::Linear(input_layer, hidden_layers[0]));
					network->push_back(torch::nn::ReLU());

					// Hidden layers
					for (size_t i = 0; i < hidden_layers.size() - 1; ++i) {
						network->push_back(torch::nn::Linear(hidden_layers[i], hidden_layers[i + 1]));
						network->push_back(torch::nn::ReLU());
					}

					// Last hidden layer to output layer
					network->push_back(torch::nn::Linear(hidden_layers.back(), output_layer));
				}
			}

			// Forward pass
			torch::Tensor forward(torch::Tensor x) {
				return network->forward(x);
			}
		};
	}
#endif


	NeuralNetworkProvider::NeuralNetworkProvider(DynaPlex::MDP mdp) :
		mdp{ mdp }
	{

	}

#if DP_TORCH_AVAILABLE
	torch::nn::AnyModule NeuralNetworkProvider::GetTrainableNN(DynaPlex::VarGroup nn_config) {
		std::string type;
		nn_config.Get("type", type);
		if (type == "mlp") {
			if (!mdp->ProvidesFlatFeatures())
				throw DynaPlex::Error("NeuralNetworkProvider::GetTrainableNN - cannot provide network with type \"" + type + "\" since mdp does not provide flat state features. Define GetFeatures(const State&,DynaPlex::Features) const on MDP to enable network type " + type);

			auto mlp_network_ptr = std::make_shared<NeuralNetworks::MLP>(mdp, nn_config);
			return torch::nn::AnyModule(mlp_network_ptr);
		}

		throw DynaPlex::NotImplementedError("NeuralNetworkProvider::GetTrainableNN - Type" + type + " not implemented.");
	}
#endif


}//namespace DynaPlex