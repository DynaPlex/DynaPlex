#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "neuralnetworkprovider.h"


// Forward declarations
namespace torch {
    namespace nn {
        class AnyModule;
    }
}

namespace DynaPlex {
    class NN_Policy : public PolicyInterface {
    public:

        DynaPlex::MDP mdp;
#if DP_TORCH_AVAILABLE
        std::unique_ptr<torch::nn::AnyModule> neural_network;
#endif
        DynaPlex::VarGroup policy_config;
        NN_Policy(DynaPlex::MDP mdp);

        std::string TypeIdentifier() const override;

        const DynaPlex::VarGroup& GetConfig() const override;

        void SetAction(std::span<Trajectory> trajectories) const override;
    };

}  // namespace DynaPlex

