#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/torchavailability.h"
namespace DynaPlex::Tests {
	

	TEST(DCL, basics) {
		auto& dp  = DynaPlexProvider::Get();
		auto& system = dp.System();
		
		DynaPlex::VarGroup config;
		//retrieve MDP registered under the id string "lost_sales":
		config.Add("id", "lost_sales");
		//add other parameters required by that MDP:
		config.Add("p", 9.0);
		config.Add("h", 1.0);
		config.Add("leadtime", 2);
		config.Add("demand_dist", DynaPlex::VarGroup({
			{"type", "poisson"},
			{"mean", 4.0}
			}));

		DynaPlex::MDP mdp = dp.GetMDP(config);

		

		auto policy = mdp->GetPolicy("base_stock");



		//configuration of neural network training:
		DynaPlex::VarGroup nn_training{
			{"mini_batch_size", 2},
			{"early_stopping_patience", 10},
			//So that test runs quickly:
			{"max_training_epochs",1 }
		};
		int64_t num_gens = 2;
		//Set very low numbers so that the test runs quickly:
		DynaPlex::VarGroup dcl_config{
			{"N",3},//number of samples
			{"M",1},//rollouts per action
			{"H",3},//horizon, i.e. number of steps.
			{"num_gens",num_gens},//number of neural network generations.
			{"nn_training",nn_training},
			{"silent",true }//to ensure that DCL does not write any output to console				
		};
		DynaPlex::Algorithms::DCL dcl = dp.GetDCL(mdp, policy, dcl_config);


		if (DynaPlex::TorchAvailability::TorchAvailable())
		{
			EXPECT_NO_THROW(dcl.TrainPolicy());
			std::vector<DynaPlex::Policy> policies;
			EXPECT_NO_THROW(
				policies =dcl.GetPolicies();
			);

			// Test sequential halving
			{
				dcl_config.Set("M", 10); // M=10 guarantees SH for up to 2^^10 allowed action per state
				DynaPlex::Algorithms::DCL dcl_sh = dp.GetDCL(mdp, policy,dcl_config);
				EXPECT_NO_THROW(dcl_sh.TrainPolicy());
			}


			auto loc = system.filepath("test", "t_dcl", "basics", "policy_gen" + std::to_string(0));
			//note that generation 0 corresponds to a policy type that cannot be saved.
			EXPECT_THROW(
				dp.SavePolicy(policies[0], loc), DynaPlex::Error
			);


			EXPECT_EQ(policies.size(), num_gens+1);
			{
				//note that generation 0 corresponds to a policy type that cannot be saved.
				int gen = 1;
				for (int gen = 1; gen < policies.size(); gen++)
				{
					auto loc = system.filepath("test", "t_dcl", "basics", "policy_gen" + std::to_string(gen));
					ASSERT_NO_THROW(
						dp.SavePolicy(policies[gen], loc);
					);
				}
			}

			config.Set("p", 90.0);
			config.Set("h", 10.0);
			//Note that this makes an internal copy of the configuration.
			//multiplying p and h by 10 will ensure this MDP will have the same upper bound on orders
			//with same leadtime, it will have same input and output dimensions.
			auto mdp_same_input_output = dp.GetMDP(config);
			//Now change the leadtime, to change input dimensions:
			config.Set("leadtime", 3);
			auto incompatible_mdp = dp.GetMDP(config);

			auto mdp_different_input = dp.GetMDP(config);

			for (int gen=1;gen<policies.size();gen++)
			{
				auto loc = system.filepath("test", "t_dcl", "basics", "policy_gen" + std::to_string(gen));
				DynaPlex::Policy loaded,loaded2,loaded3;
				ASSERT_NO_THROW(
			    	loaded = dp.LoadPolicy(mdp, loc);
					loaded2 = dp.LoadPolicy(mdp_same_input_output, loc);
				);
				//different input dimensions, will not be able to load a policy from this mdp 
				//from the trained neural network info:
				ASSERT_THROW(
					loaded3 = dp.LoadPolicy(mdp_different_input, loc), DynaPlex::Error
				);

			}
						
		}
		else
		{
			EXPECT_NO_THROW(
			try
			{
				dcl.TrainPolicy();
			}
			catch (const DynaPlex::Error& e)
			{
				std::string expected_exception = "DynaPlex: PolicyTrainer::TrainPolicy - Torch not available, cannot train policy. To make torch available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt ";
				EXPECT_EQ(e.what(), expected_exception);
			}
			);
		}
		

	}


}