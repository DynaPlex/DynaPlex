#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/Demonstrator.h"
namespace DynaPlex::Tests {
	

	TEST(Demonstrator, WithLostSales) {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.GetSystem();

		std::string model_name = "lost_sales";
		std::string mdp_config_name = "mdp_config_0.json";
		//configure MDP:
		ASSERT_TRUE(
			system.file_exists("mdp_config_examples", model_name, mdp_config_name)
		);
		DynaPlex::VarGroup mdp_vars_from_json;
		ASSERT_NO_THROW(
			std::string file_path = system.filename("mdp_config_examples", model_name, mdp_config_name);
		    mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		);

		auto mdp = dp.GetMDP(mdp_vars_from_json);
		
		DynaPlex::Policy policy = mdp->GetPolicy("random");

		int64_t max_events = 10;
		auto demonstrator_config = DynaPlex::VarGroup{ {"max_event_count", max_events},{"seed",123}};
		auto demonstrator = dp.GetDemonstrator(demonstrator_config);
		auto trace = demonstrator.GetTrace(mdp);
		
		for (auto& elem : trace)
		{//this would print the various parts of the trace. 
		//	std::cout << elem.Dump() << std::endl;
		}

		//lost_sales starts with action, and alternates between actions and events, never final. Hence, there will be 2*maxevents elements in trace. 
		ASSERT_EQ(trace.size(), max_events*2);
	}
}