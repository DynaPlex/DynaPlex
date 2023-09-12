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
	
		DynaPlex::Erasure::ActionRangeProvider<t_MDP> provider;
	public:

		RandomPolicy(std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& varGroup)
			: provider{mdp}
		{
		}
		using State = t_MDP::State;

		int64_t GetAction(const State& state, DynaPlex::RNG& rng) const
		{
			int64_t numAllowedActions{ 0 };
			for (const int64_t action: provider(state))
			{				
				numAllowedActions++;				
			}
			if (numAllowedActions == 0)
			{
				throw DynaPlex::Error("RandomPolicy: Not a single action allowed.");
			}
			double d_budget = rng.genUniform() * static_cast<double>(numAllowedActions);
			int64_t budget = static_cast<int64_t>(d_budget);

			for (const int64_t action : provider(state))
			{	
				if (budget == 0)
				{
					return action;
				}
				budget--;
							
			}
			throw DynaPlex::Error("RandomPolicy: Error in logic.");
		}
	};
}