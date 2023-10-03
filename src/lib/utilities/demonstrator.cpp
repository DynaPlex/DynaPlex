#include "dynaplex/demonstrator.h"
#include "dynaplex/trajectory.h"
namespace DynaPlex :: Utilities{

	Demonstrator::Demonstrator(const System& system,const VarGroup& config)
		:system{system}
	{
		config.GetOrDefault("max_period_count", max_period_count, 3);
		config.GetOrDefault("rng_seed", rng_seed, 0);
	}
	std::vector<VarGroup> Demonstrator::GetTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy)
	{
		std::vector<VarGroup> trace;
		if (!mdp)
		{
			throw DynaPlex::Error("Demonstrator: MDP should not be null");
		}
		if (!policy)
		{   //default to random policy. 
			policy = mdp->GetPolicy("random");
		}
		//vector that will hold a single trajectory. 
		Trajectory trajectory{ mdp->NumEventRNGs()};
		mdp->InitiateState({ &trajectory,1 });
		trajectory.SeedRNGProvider(system, true,rng_seed);
		double cumulative_return = 0.0;
		bool finalreached = false;
		while (trajectory.PeriodCount < max_period_count && !finalreached)
		{
			VarGroup snapshot{};
			snapshot.Add("state", trajectory.GetState()->ToVarGroup() );
			snapshot.Add("period_count", trajectory.PeriodCount);
			snapshot.Add("incr_return", trajectory.CumulativeReturn-cumulative_return);
			snapshot.Add("cum_return", trajectory.CumulativeReturn);
			cumulative_return = trajectory.CumulativeReturn;

			auto& cat = trajectory.Category;
			if (cat.IsAwaitEvent())
				mdp->IncorporateEvent({ &trajectory,1 });
			else if (cat.IsAwaitAction())
			{
				policy->SetAction({ &trajectory,1 });
				snapshot.Add("action", trajectory.NextAction);
				mdp->IncorporateAction({ &trajectory,1 });
			}
			else if (cat.IsFinal())
			{
				finalreached = true;
			}
			trace.push_back(snapshot);
		}

		return trace;
	}
}