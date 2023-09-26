#include "testutils.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/_demonstrator.h"
#include <gtest/gtest.h>

namespace DynaPlex::Tests {
	
	


    void Tester::ExecuteTest(const std::string& model_name, const std::string& mdp_config_name, const std::string& policy_config_name) {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.System();
		std::string info = model_name + "|" + mdp_config_name;
		if (policy_config_name != "")
			info += "|" + policy_config_name;
		info += ": ";
		//configure MDP:
		ASSERT_TRUE(
			system.file_exists("mdp_config_examples", model_name, mdp_config_name) 
		)<<info << "Does a file models/models/" << model_name << "/" << mdp_config_name << "exist, and was it succesfullly copied to IO_DynaPlex folder by CMake?";


		DynaPlex::VarGroup mdp_vars_from_json;
		DynaPlex::MDP mdp;
		ASSERT_NO_THROW(
			std::string file_path = system.filename("mdp_config_examples", model_name, mdp_config_name);
		    mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		) << info << "File: models/models/" << model_name << "/" << mdp_config_name << " is not accepted as a validly formatted configuration file.";

		ASSERT_NO_THROW(
			mdp = dp.GetMDP(mdp_vars_from_json);
		) << info << "Failed to configure mdp with file models/models/" << model_name << "/" << mdp_config_name << ". Is the id correct? Were all neccesary arguments in MDP constructor provided in json file, and are they valid? Does GetStaticInfo provide all required information?";
		ASSERT_TRUE(mdp) <<info << "Contact developers. ";

		ASSERT_EQ(
			mdp->TypeIdentifier(), model_name
		) << info << "File models/models/" << model_name << "/" << mdp_config_name << " does not define a mdp of type " << model_name;

		DynaPlex::Policy policy;
		//configure policy:
		if (policy_config_name != "")
		{
			ASSERT_TRUE(
				system.file_exists("mdp_config_examples", model_name, policy_config_name)
			) <<info << "File models/models/" << model_name << " / " << mdp_config_name << " does not exist";
			
			VarGroup policy_vars_from_json;
			ASSERT_NO_THROW(
				std::string file_path = system.filename("mdp_config_examples", model_name, policy_config_name);
				policy_vars_from_json = VarGroup::LoadFromFile(file_path);
			) << info << "File: models/models/" << model_name << "/" << policy_config_name << " is not accepted as a validly formatted configuration file.";


			ASSERT_NO_THROW(
				policy = mdp->GetPolicy(policy_vars_from_json);
			) << info << "Failed to configure policy with file models/models/" << model_name << "/" << policy_config_name << ". Is the id correct? Were all neccesary arguments in policy constructor provided in json file? Did you correctly register the custom policy in RegisterPolicies?";
			
		}
		else
		{  //default to random:
			ASSERT_NO_THROW(
				policy = mdp->GetPolicy("random");
			) <<info;
		}
		std::string policy_type = "";
		ASSERT_NO_THROW(
			policy_type = policy->TypeIdentifier();
		) << info ;

		//policy must now be initiated. 
		ASSERT_TRUE(policy) <<info;		

		int64_t numEventTrajectories{};
		ASSERT_NO_THROW(
			numEventTrajectories = mdp->NumEventRNGs();
		);
		Trajectory trajectory{ numEventTrajectories };

		int numSeeds = 64;
		std::vector<DynaPlex::dp_State> someStates;
		someStates.reserve(numSeeds + 1);
		for (int seed = 0; seed < numSeeds; seed++)
		{
			ASSERT_NO_THROW(
				mdp->InitiateState({ &trajectory,1 });
			) << info << "Did you correctly implement GetInitialState() const or GetInitialState(DynaPlex::RNG&) const";

			if (seed == 0)
			{
				ASSERT_NO_THROW(
					someStates.push_back(trajectory.GetState()->Clone())
				) << info << "Error while cloning state. Does mdp::State support copying?";
			}
			ASSERT_NO_THROW(
				trajectory.SeedRNGProvider(dp.System(), true, seed);
			) << info;

			int64_t max_event_count = 128;
			bool finalreached = false;
			int64_t action_count = 0;
			
			
			while (trajectory.EventCount < max_event_count && !finalreached)
			{
				auto& cat = trajectory.Category;
				if (cat.IsAwaitEvent())
				{
					ASSERT_NO_THROW(
						mdp->IncorporateEvent({ &trajectory,1 });
					) << info << "Did you correctly implement ModifyStateWithEvent  and GetEvent?";
				}
				else if (cat.IsAwaitAction())
				{
					//std::cout << "test" << std::endl;
					action_count++;
					ASSERT_NO_THROW(
						policy->SetAction({ &trajectory,1 });
					) << info << " Issue with policy. Did you correctly implement GetAction on policy " + policy->TypeIdentifier() + "?";
					ASSERT_NO_THROW(
						mdp->IncorporateAction({ &trajectory,1 })
					) << info << "Did you correctly implement ModifyStateWithAction? ";
				}
				else if (cat.IsFinal())
				{
					finalreached = true;
				}
				if (!RelaxOnProgramFlow)
				{
					//getting stuck in an action loop should trip this. 
					ASSERT_LE(action_count, max_event_count * 1000) << info << "A simulation of your trajection seems to get stuck in a loop with only actions, and no events. In ModifyStateWithAction, did you ensure that the state category becomes AwaitEvent or Final? If this is intentional, set RelaxOnLoops to skip this test.";
				}
				else
				{
					if (action_count > 100 * max_event_count)
					{
						finalreached = true;
					}
				}
			}

			if (!RelaxOnProgramFlow)
			{
				//expect at least a single action per event- probably more. 
				ASSERT_GE(action_count, trajectory.EventCount / 2 - 1) << info << "Substantially less actions then anticipated were taken. Did you ensure that State Category in ModifyStateWithEvent is set to AwaitAction again? If this is intentional, set RelaxOnLoops to skip this test. ";
			}
			ASSERT_NO_THROW(
				someStates.push_back(trajectory.GetState()->Clone())
			) << info << "Error while cloning state. Does mdp::State support copying?";
		}


		ASSERT_EQ(someStates.size(), numSeeds + 1);

		if (!SkipEqualityTests)
		{
			ASSERT_TRUE(mdp->SupportsEqualityTest());
			auto& firstState = someStates[0];
			for (auto& state : someStates)
			{
				ASSERT_TRUE(mdp->StatesAreEqual(state, state));
			}
		}

		if (!SkipStateSerializationTests)
		{
			auto& firstState = someStates[0];

			for (auto& state : someStates)
			{
				auto vars = state->ToVarGroup();

				DynaPlex::dp_State loaded_state{};
				
				ASSERT_NO_THROW(
					loaded_state = mdp->GetState(vars);
				) << "Issue with state serialization; are MDP::GetState(const VarGroup& vars) const and MDP::State::ToVarGroup() const correctly implemented? Set SkipStateSerializationTests to skip this test.  ";
		
			auto vars_from_loaded_state = loaded_state->ToVarGroup();
				if (!SkipEqualityTests)
				{
					ASSERT_TRUE(mdp->StatesAreEqual(state, loaded_state)) << "Issue with state serialization; are MDP::GetState(const VarGroup& vars) const and MDP::State::ToVarGroup() const correctly implemented? Set SkipStateSerializationTests to skip this test.  ";
				}

				ASSERT_EQ(vars, vars_from_loaded_state) << "Issue with state serialization; are MDP::GetState(const VarGroup& vars) const and MDP::State::ToVarGroup() const correctly implemented? Set SkipStateSerializationTests to skip this test. ";



				std::vector<Trajectory> trajVec{};
				for (size_t i = 0; i < 2; i++)
				{
					trajVec.push_back(std::move(Trajectory(mdp->NumEventRNGs(),0)));
					
				} 

				mdp->InitiateState({ &trajVec[0] ,1 }, state);
				mdp->InitiateState({ &trajVec[1] ,1 }, loaded_state);
				trajVec[0].SeedRNGProvider(dp.System(), false,12,0);
				trajVec[1].SeedRNGProvider(dp.System(), false, 12, 0);

				int64_t max_event_count = 10;
				int64_t action_count = 0;
				
				
				bool finalreached = false;
				while (trajVec[0].EventCount < max_event_count && !finalreached)
				{
					auto& cat = trajVec[0].Category;
					ASSERT_EQ(trajVec[0].Category, trajVec[1].Category)<<info<<"Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					ASSERT_EQ(trajVec[0].CumulativeReturn, trajVec[1].CumulativeReturn) << info << "Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					ASSERT_EQ(trajVec[0].EventCount, trajVec[1].EventCount) << info << "Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					
					if (!SkipEqualityTests)
					{
						ASSERT_TRUE(mdp->StatesAreEqual(trajVec[0].GetState(), trajVec[1].GetState())) << info << "Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					}


					if (cat.IsAwaitEvent())
					{
						ASSERT_NO_THROW(
							mdp->IncorporateEvent(trajVec);
						) << info << "Did you correctly implement ModifyStateWithEvent  and GetEvent?";
					}
					else if (cat.IsAwaitAction())
					{
						//std::cout << "test" << std::endl;
						action_count++;
						ASSERT_NO_THROW(
							policy->SetAction(trajVec);
						) << info << " Issue with policy. Did you correctly implement GetAction on policy " + policy->TypeIdentifier() + "?";
						ASSERT_NO_THROW(
							mdp->IncorporateAction(trajVec)
						) << info << "Did you correctly implement ModifyStateWithAction? ";
					}
					else if (cat.IsFinal())
					{
						finalreached = true;
					}
					if (!RelaxOnProgramFlow)
					{
						//getting stuck in an action loop should trip this. 
						ASSERT_LE(action_count, max_event_count * 1000) << info << "A simulation of your trajection seems to get stuck in a loop with only actions, and no events. In ModifyStateWithAction, did you ensure that the state category becomes AwaitEvent or Final? If this is intentional, set RelaxOnLoops to skip this test. ";
					}
					else
					{
						if (action_count > 100 * max_event_count)
						{
							finalreached = true;
						}
					}
				}
			}
		}
    }
}