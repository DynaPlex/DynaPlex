#pragma once
#include "dynaplex/policy.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/statecategory.h"
#include "stateadapter.h"
#include "erasure_concepts.h"

namespace DynaPlex::Erasure
{
	template<typename t_MDP, typename t_Policy>
	class PolicyAdapter final : public PolicyInterface
	{
		static_assert(HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		static_assert(HasGetStateCategory<t_MDP>, "MDP must publicly define a function GetStateCategory(const MDP::State) that returns a StateCategory");
		using t_State = t_MDP::State;

		static_assert(HasGetAction<t_Policy, t_State> ^ HasGetActionRNG<t_Policy, t_State> ,
			" t_MDP should implement GetAction(State) or GetAction(State,RNG), but not both!");

		t_Policy policy;
		std::shared_ptr<const t_MDP> mdp;
		std::string identifier;
		int64_t mdp_int_hash;
	public:
		std::string Identifier() const override
		{
			return identifier;
		}

		PolicyAdapter(std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& policy_vars, const int64_t mdp_int_hash )
			:mdp{mdp},
			policy{mdp,policy_vars },
			identifier{ policy_vars.Identifier()},
			mdp_int_hash{mdp_int_hash}
		{

		}

		void SetActions(std::span<Trajectory> trajectories) const override
		{	
			for (Trajectory& traj: trajectories)
			{
				// Check that the states belong to this MDP
				if (traj.GetState()->mdp_int_hash != mdp_int_hash)
				{
					throw DynaPlex::Error("Error in Policy->SetActions: It seems you tried to call with states not"
						"associated with the MDP that this policy was obtained from. Please note that policies, even "
						"generic ones, can only act on states from the same mdp instance that the policy was obtained from.");
				}
				
				const DynaPlex::StateCategory& cat = traj.Category; 
				
				if (cat.IsAwaitAction())
				{
					//convert type-erased state to underlying type. 
					StateAdapter<t_State>* adapter = static_cast<StateAdapter<t_State>*>(traj.GetState().get());
					t_State& state = adapter->state;
					//dispatch to policy, depending on the signature of the GetAction implemented
					//on the policy
					if constexpr (HasGetAction<t_Policy, t_State>)
					{
						traj.NextAction = policy.GetAction(state);
					}
					else
					{
						RNG& rng = traj.RNGProvider.GetPolicyRNG();
						traj.NextAction = policy.GetAction(state, rng);
					}
				}
				else
				{
					throw DynaPlex::Error("Error in Policy->SetActions: Cannot set action when Trajectory.Category is not IsAwaitAction, i.e. when state is not IsAwaitAction.");
				}
			}

			
		}

	};
}
