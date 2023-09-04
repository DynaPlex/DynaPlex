#pragma once
#include "dynaplex/policy.h"
#include "dynaplex/states.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "statesadapter.h"

namespace DynaPlex::Erasure
{
	template<typename t_MDP, typename t_Policy>
	class PolicyAdapter : public PolicyInterface
	{
		using State = t_MDP::State;
		t_Policy policy;
		std::string identifier;
		int64_t mdp_int_hash;
	public:
		std::string Identifier() const override
		{
			return identifier;
		}

		PolicyAdapter(std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& policy_vars, const int64_t mdp_int_hash )
			:policy{mdp,policy_vars },
			identifier{ policy_vars.Identifier()},
			mdp_int_hash{mdp_int_hash}
		{

		}

		virtual void SetActions(const DynaPlex::States& states, std::vector<StateContext>& context) const override
		{
			// Check that the states belong to this MDP
			if (states->mdp_int_hash != mdp_int_hash)
			{
				throw DynaPlex::Error("Error in Policy->SetActions: It seems you tried to call with states not associated with the MDP that this policy was obtained from. Please note that policies, even generic ones, can only act on states from the same mdp instance that the policy was obtained from.");
			}
			//Get the State from the states. 
			std::vector<State>& vec = static_cast<StatesAdapter<State>*>(states.get())->get();
			if (vec.size() != context.size())
			{
				throw DynaPlex::Error("Error in Policy->SetActions: context and states of unequal size.");

			}
		}

	};
}
