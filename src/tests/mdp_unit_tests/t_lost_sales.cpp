#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/demonstrator.h"
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {
	
	
	TEST(lost_sales, mdp_config_0) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "lost_sales";
		std::string config_name = "mdp_config_0.json";
		Tester tester{};
		tester.ExecuteTest(model_name, config_name);
	}
	TEST(lost_sales, mdp_config_1) {
		std::string model_name = "lost_sales";
		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string mdp_config_name = "mdp_config_1.json";
		//Note: models/model_name/policy_config_name is valid json config file for a policy for "model_name".  
		std::string policy_config_name = "policy_config_1.json";
		Tester tester{};
		tester.ExecuteTest(model_name, mdp_config_name, policy_config_name);
	}

	TEST(lost_sales, Basics) {
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