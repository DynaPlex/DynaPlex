#include "dynaplex/_demonstrator.h"
#include "dynaplex/trajectory.h"
namespace DynaPlex :: Utilities{

	Demonstrator::Demonstrator(const System& system,const VarGroup& varGroup)
		:system{system}
	{
		varGroup.Get("max_event_count", max_event_count);
		if (varGroup.HasKey("seed"))
		{
			varGroup.Get("seed", seed);
		}
		else
		{
			seed = 0;
		}
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
			if (!policy)
			{
				throw DynaPlex::Error("Demonstrator: Could not retrieve default random policy");
			}
		}
		//vector that will hold a single trajectory. 
		Trajectory trajectory{ mdp->NumEventRNGs()};
		
		mdp->InitiateState({ &trajectory,1 });

		trajectory.SeedRNGProvider(system, true,seed);
		

		

		double cumulative_return = 0.0;
		bool finalreached = false;
		while (trajectory.EventCount < max_event_count && !finalreached)
		{
			VarGroup snapshot{};
			snapshot.Add("state", trajectory.GetState()->ToVarGroup() );
			snapshot.Add("event_count", trajectory.EventCount);
			snapshot.Add("incremental_return", trajectory.CumulativeReturn-cumulative_return);
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