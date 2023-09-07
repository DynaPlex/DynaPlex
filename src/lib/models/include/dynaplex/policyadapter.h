#pragma once
#include "dynaplex/policy.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "stateadapter.h"
#include "mdp_adapter_helpers/mdpadapter_concepts.h"

namespace DynaPlex::Erasure
{
	template<typename t_MDP, typename t_State>
	concept HasGetAction = requires(const t_MDP mdp, const t_State & state) {
		{ mdp.GetAction(state) } -> std::same_as<int64_t>;
	};

	template<typename t_MDP, typename t_State>
	concept HasGetActionRNG = requires(const t_MDP mdp, const t_State & state, DynaPlex::RNG & rng) {
		{ mdp.GetAction(state, rng) } -> std::same_as<int64_t>;
	};

	template<typename t_MDP, typename t_Policy>
	class PolicyAdapter final : public PolicyInterface
	{


		//static_assert(DynaPlex::Concepts::HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		using t_State = t_MDP::State;
		//static_assert(DynaPlex::Concepts::HasGetAction<t_MDP, t_State>^ DynaPlex::Concepts::HasGetActionRNG<t_MDP, t_State>,
		//	" t_MDP should implement GetAction(State) or GetAction(State,RNG), but not both!");

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

		void SetActions(std::vector<Trajectory>& trajectories) const override
		{
			
			
			for (Trajectory& traj: trajectories)
			{
				// Check that the states belong to this MDP
				if (traj.State->mdp_int_hash != mdp_int_hash)
				{
					throw DynaPlex::Error("Error in Policy->SetActions: It seems you tried to call with states not associated with the MDP that this policy was obtained from. Please note that policies, even generic ones, can only act on states from the same mdp instance that the policy was obtained from.");
				}
				StateAdapter<t_State>* stateAdapter = static_cast<StateAdapter<t_State>*>(traj.State.get());
				
			//	if (traj.Category.IsAwaitAction())
				{
			//		traj.NextAction = policy->GetAction(stateAdapter->state);
				}			
			//	else
				{
			//		throw DynaPlex::Error("Error in Policy->SetActions: Cannot set action when state is not awaitaction.");

				}
			}

			
		}

	};
}
