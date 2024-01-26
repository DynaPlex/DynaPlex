#include "dynaplex/demonstrator.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/rng.h"
namespace DynaPlex :: Utilities{


	VarGroup TraceElement::ToVarGroup() const {
		VarGroup vg;
		vg.Add("state", state->ToVarGroup());  // Assuming dp_State's pointed object has a ToVarGroup() method
		vg.Add("period_count", period_count);
		vg.Add("incr_return", incr_return);
		vg.Add("cum_return", cum_return);
		if (cat.IsAwaitAction())
		{
			vg.Add("action", action);
		}
		return vg;
	}

	Demonstrator::Demonstrator(const System& system,const VarGroup& config)
		:system{system}
	{
		config.GetOrDefault("max_period_count", max_period_count, 3);
		config.GetOrDefault("rng_seed", rng_seed, 11112014);
		if (rng_seed < 0)
			throw DynaPlex::Error("Demonstrator :: Invalid rng_seed - should be non-negative");		
	}

	std::vector<TraceElement> Demonstrator::GetObjectTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy) {
		std::vector<TraceElement> trace;

		if (!mdp) {
			throw DynaPlex::Error("Demonstrator: MDP should not be null");
		}

		if (!policy) {
			// Default to random policy. 
			policy = mdp->GetPolicy("random");
		}

		// Vector that will hold a single trajectory.
		Trajectory trajectory{};
		trajectory.RNGProvider.SeedEventStreams(true, rng_seed);
		mdp->InitiateState({ &trajectory,1 });
		
		double cumulative_return = 0.0;
		bool final_reached = false;

		while (trajectory.PeriodCount < max_period_count && !final_reached) {
			TraceElement element;

			element.state = trajectory.GetState()->Clone();
			element.period_count = trajectory.PeriodCount;
			element.incr_return = trajectory.CumulativeReturn - cumulative_return;
			element.cum_return = trajectory.CumulativeReturn;
			cumulative_return = trajectory.CumulativeReturn;

			auto& cat = trajectory.Category;
			element.cat = cat;
			if (cat.IsAwaitEvent()) {
				mdp->IncorporateEvent({ &trajectory,1 });
				element.action = 0;
			}
			else if (cat.IsAwaitAction()) {
				policy->SetAction({ &trajectory,1 });
				element.action = trajectory.NextAction;
				mdp->IncorporateAction({ &trajectory,1 });
			}
			else if (cat.IsFinal()) {
				final_reached = true;
				element.action = 0;
			}

			trace.push_back(std::move(element)); 
		}

		return trace;
	}

	std::vector<VarGroup> Demonstrator::GetTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy)
	{
		// Use the GetObjectTrace to get the vector of TraceElement
		auto objectTrace = GetObjectTrace(mdp, policy);

		// Transform the TraceElement objects to VarGroup
		std::vector<VarGroup> trace;
		trace.reserve(objectTrace.size());
		for (const auto& element : objectTrace) {
			trace.push_back(element.ToVarGroup());
		}

		return trace;
	}
}