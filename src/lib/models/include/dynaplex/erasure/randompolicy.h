#pragma once
#include "memory"
#include "dynaplex/rng.h"
#include "dynaplex/vargroup.h"
#include "erasure_concepts.h"
#include "actionrangeprovider.h"


namespace DynaPlex::Erasure
{
	template <class t_MDP>
	class RandomPolicy
	{
		static_assert(HasGetStaticInfo<t_MDP>, "MDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");
		//This is needed as somebody needs to own the mdp, and provider is non-owning. 
		std::shared_ptr<const t_MDP> mdp;
		DynaPlex::Erasure::ActionRangeProvider<t_MDP> provider;
	public:

		RandomPolicy(std::shared_ptr<const t_MDP> mdp_, const DynaPlex::VarGroup& varGroup)
			:mdp{ mdp_ },
		     provider{*mdp.get()}
		{
		}
		using State = t_MDP::State;

		int64_t GetAction(const State& state, DynaPlex::RNG& rng) const
		{
			int64_t numAllowedActions;
			for (const int64_t action: provider(state))
			{				
				numAllowedActions++;				
			}
			if (numAllowedActions == 0)
			{
				throw DynaPlex::Error("RandomPolicy: Not a single action allowed.");
			}
			double budget = rng.genUniform() * numAllowedActions;
			for (const int64_t action : provider(state))
			{			
				budget--;
				if (budget <= 0.0)
				{
					return action;
				}			
			}
			throw DynaPlex::Error("RandomPolicy: Error in logic.");
		}
	};
}