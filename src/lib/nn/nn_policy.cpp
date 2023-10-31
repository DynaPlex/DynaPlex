#include "nn_policy.h"
#include "dynaplex/system.h"
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#endif
namespace DynaPlex {

    NN_Policy::NN_Policy(DynaPlex::MDP mdp)
        : mdp(mdp) {
      
    }

    std::string NN_Policy::TypeIdentifier() const {
        return "NN_Policy";
    }

    const DynaPlex::VarGroup& NN_Policy::GetConfig() const {
        return policy_config;
    }

    void NN_Policy::SetAction(std::span<Trajectory> trajectories) const {
#if DP_TORCH_AVAILABLE
        int input_dim = mdp->NumFlatFeatures();

        // Convert trajectories into a tensor for the neural network.
        torch::Tensor batched_inputs = torch::empty({ static_cast<int64_t>(trajectories.size()), input_dim }, torch::kFloat32);
        float* input_data_ptr = batched_inputs.data_ptr<float>();

        mdp->GetFlatFeatures(trajectories, std::span<float>(input_data_ptr, trajectories.size() * input_dim));

        torch::NoGradGuard no_grad;
        // Pass the tensor through the neural network to get scores for actions.
        torch::Tensor output_scores = neural_network->forward(batched_inputs);

        // Use MDP's SetArgMaxAction to determine the action based on the neural network's scores.
        mdp->SetArgMaxAction(trajectories, std::span<float>(output_scores.data_ptr<float>(), trajectories.size() * mdp->NumValidActions()));
#else
        throw DynaPlex::Error("NN_Policy: Torch not available - Cannot SetAction. To make torch available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt. ");
#endif
    }

}  // namespace DynaPlex
