#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/Demonstrator.h"
namespace DynaPlex::Tests {
	

	TEST(Demonstrator, WithLostSales) {
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
		    policy = mdp->GetPolicy("base_stock");
		);
		


		int64_t max_events = 100;
		auto demonstrator_config = DynaPlex::VarGroup{ {"max_event_count", max_events},{"seed",123}};

		auto demonstrator = dp.GetDemonstrator(demonstrator_config);

		auto trace = demonstrator.GetTrace(mdp);
		
		for (auto& elem : trace)
		{
		//	std::cout << elem.Dump() << std::endl;
		}

		//lost_sales starts with action, and alternates between actions and events, never final. Hence, there will be 2*maxevents elements in trace. 
		ASSERT_EQ(trace.size(), max_events*2);
	}
}