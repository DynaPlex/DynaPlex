#include "TestUtils.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/Demonstrator.h"
#include <gtest/gtest.h>

namespace DynaPlex::Tests {
	
    void ExecuteTest(const std::string& model_name, const std::string& mdp_config_name, const std::string& policy_config_name) {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.GetSystem();

		//configure MDP:
		ASSERT_TRUE(
			system.file_exists("mdp_config_examples", model_name, mdp_config_name)
		);
		DynaPlex::VarGroup mdp_vars_from_json;
		DynaPlex::MDP mdp;
		ASSERT_NO_THROW(
			std::string file_path = system.filename("mdp_config_examples", model_name, mdp_config_name);
			mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		);

		ASSERT_NO_THROW(
			mdp = dp.GetMDP(mdp_vars_from_json);
		);
		ASSERT_TRUE(mdp);


		DynaPlex::Policy policy;
		//configure policy:
		if (policy_config_name != "")
		{
			ASSERT_TRUE(
				system.file_exists("mdp_config_examples", model_name, policy_config_name)
			);
			
			VarGroup policy_vars_from_json;
			ASSERT_NO_THROW(
				std::string file_path = system.filename("mdp_config_examples", model_name, policy_config_name);
			    policy_vars_from_json = VarGroup::LoadFromFile(file_path);
			);

			ASSERT_NO_THROW(
				policy = mdp->GetPolicy(policy_vars_from_json);
			);
		}
		else
		{  //default to random:
			ASSERT_NO_THROW(
				policy = mdp->GetPolicy("random");
			);
		}
		//policy must now be initiated. 
		ASSERT_TRUE(policy);
		

		int64_t numEventTrajectories;
		ASSERT_NO_THROW(
			numEventTrajectories = mdp->NumEventRNGs();
		);
		Trajectory trajectory{ numEventTrajectories };


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