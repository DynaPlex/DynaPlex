#pragma once
#include<vector>
#include "dynaplex/state.h"
#include "dynaplex/vargroup.h"

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

        Sample() = default;
        Sample(int64_t action_label, DynaPlex::dp_State state);



        DynaPlex::VarGroup ToVarGroup() const;

    };
}
