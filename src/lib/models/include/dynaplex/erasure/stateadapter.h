#pragma once
#include "dynaplex/state.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Erasure {

    template <typename t_State>
    class StateAdapter final : public StateBase {
    public:
        t_State state;
        StateAdapter(int64_t hash_value, const t_State& s)
            : StateBase(hash_value), 
            state(s) 
        {
        }      
        
        VarGroup ToVarGroup() const override
        {
            if constexpr (DynaPlex::Concepts::ConvertibleToVarGroup<t_State>)
            {
                return state.ToVarGroup();
            }
            else
                throw DynaPlex::Error("State->ToVarGroup() : State is not ConvertibleToVarGroup.");

        }

        std::unique_ptr<StateBase> Clone() const override {
            return std::make_unique<StateAdapter>(*this);
        }
    };

} 