#pragma once
#include "dynaplex/rng.h";
#include "mdpadapter_concepts.h";
#include "actionrangeprovider.h";
#include "memory";
#include "dynaplex/vargroup.h";

namespace DynaPlex::Erasure::Helpers
{
	template <class t_MDP>
	class RandomPolicy
	{
		static_assert(DynaPlex::Concepts::HasGetStaticInfo<t_MDP>, "MDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");

		//	DynaPlex::ActionRangeProvider<t_MDP> provider;
		std::shared_ptr<const t_MDP> mdp;
	public:

		RandomPolicy(std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& varGroup)
			:mdp{ mdp }//,
			//	provider{mdp.get(),varGroup},

		{

		}

		using State = t_MDP::State;

		int64_t GetAction(const State& state, DynaPlex::RNG& rng) const
		{
			/*auto numActions = adapter.NumValidActions(state);
			size_t numAllowedActions{ 0 };
			for (size_t action = 0; action < numActions; action++)
			{
				if (adapter.IsAllowedAction(state, action))
				{
					numAllowedActions++;
				}
			}
			if (numAllowedActions == 0)
			{
				throw DynaPlex::Error("RandomPolicy: Not a single action allowed.");
			}
			double budget = rng.genUniform() * numAllowedActions;
			for (size_t action = 0; action < numActions; action++)
			{
				if (adapter.IsAllowedAction(state, action))
				{
					budget--;
					if (budget <= 0.0)
					{
						return action;
					}
				}
			}
			throw DynaPlex::Error("RandomPolicy: Error in logic.");*/
		}
	};
}