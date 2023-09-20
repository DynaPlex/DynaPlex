#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/Demonstrator.h"
namespace DynaPlex::Tests {
	

	TEST(MDP, Basics) {
		auto& dp = DynaPlexProvider::Get();
		DynaPlex::VarGroup vars;
		vars.Add("id", "lost_sales");
		vars.Add("p", 4.0);
		vars.Add("h", 1.0);
		vars.Add("leadtime", 3);
		vars.Add("discount_factor", 1.0);

		vars.Add("demand_dist", DynaPlex::VarGroup({
			{"type", "poisson"},
			{"mean", 4.0}
			}));



		DynaPlex::MDP mdp;
		DynaPlex::Policy policy;

		ASSERT_NO_THROW(
			mdp = dp.GetMDP(vars);
		);
		ASSERT_NO_THROW(
			policy = mdp->GetPolicy("random");
		);

		int64_t numEventTrajectories;
		ASSERT_NO_THROW(
			numEventTrajectories = mdp->NumEventRNGs();
		);
		Trajectory trajectory{numEventTrajectories};


		
		ASSERT_NO_THROW(
		);
		ASSERT_NO_THROW(
			mdp->InitiateState({ &trajectory,1 });
		);
		ASSERT_NO_THROW(
			trajectory.SeedRNGProvider(dp.GetSystem(), true, 123);
		);

	    int64_t max_event_count = 10;
		bool finalreached = false;
		while (trajectory.EventCount < max_event_count && !finalreached)
		{
			auto& cat = trajectory.Category;
			if (cat.IsAwaitEvent())
			{
				ASSERT_NO_THROW(
					mdp->IncorporateEvent({ &trajectory,1 });
				);
			}
			else if (cat.IsAwaitAction())
			{
				ASSERT_NO_THROW(
					policy->SetAction({ &trajectory,1 });
				);
				ASSERT_NO_THROW(
					mdp->IncorporateAction({ &trajectory,1 });
				);
			}
			else if (cat.IsFinal())
			{
				finalreached = true;
			}
		}			
	}
}