#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/Demonstrator.h"
namespace DynaPlex::Tests {
	

	TEST(LostSales, Basics) {
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
		    policy = mdp->GetPolicy("basestock");
		);
		
		int64_t max_events = 400;
		auto demonstrator_config = DynaPlex::VarGroup{ {"max_event_count", max_events},{"seed",123}};

		auto demonstrator = dp.GetDemonstrator(demonstrator_config);

		auto trace = demonstrator.GetTrace(mdp);
		
		ASSERT_EQ(trace.size(), max_events*2-1);
	}
}