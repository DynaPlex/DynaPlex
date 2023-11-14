#pragma once
#include<vector>
#include "dynaplex/state.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/mdp.h"
namespace DynaPlex::NN
{
    /// A state with a sample action or distribution over actions, for use with supervised learning algorithms.
    class Sample
    {
    public:

        //when adding other members, do not forget to check for "if (std::isnan(zValue) || std::isinf(zValue))" to avoid pain when serializing. 
        int64_t action_label{ 0 };
        DynaPlex::dp_State state;
        //for implementation verification purposes.
        int64_t sample_number;
        std::vector<double> q_hat_vec;
        double z_stat;
        double q_hat;
        std::vector<double> cost_improvement;
        std::vector<double> probabilities;

        Sample() = default;
        Sample(int64_t action_label, DynaPlex::dp_State state);



        DynaPlex::VarGroup ToVarGroup() const;

        DynaPlex::VarGroup ToVarGroupWithFeats(DynaPlex::MDP mdp) const;
    };
}
