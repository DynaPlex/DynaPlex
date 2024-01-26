#include "testutils.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/demonstrator.h"
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
			std::string file_path = system.filepath("mdp_config_examples", model_name, mdp_config_name);
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
				std::string file_path = system.filepath("mdp_config_examples", model_name, policy_config_name);
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
			

		Trajectory trajectory{ };


		ASSERT_GE(NumParallelTests, 1) << info << "tester.NumParallelTests should be >=1";
		int numSeeds = NumParallelTests;
		std::vector<DynaPlex::dp_State> someStates;
		someStates.reserve(numSeeds + 1);
		bool ProvidesFlatFeatures{ false };
		int64_t NumFlatFeatures{ 0 };

		if (AssertFlatFeatureAvailability)
		{
			ASSERT_TRUE(
				mdp->ProvidesFlatFeatures()
			);
		}

		ASSERT_NO_THROW(
			ProvidesFlatFeatures = mdp->ProvidesFlatFeatures();
		);

		if (ProvidesFlatFeatures)
		{
			ASSERT_NO_THROW(
				NumFlatFeatures = mdp->NumFlatFeatures();
			);
		}

		std::vector<float> feats_store(NumFlatFeatures, 0.0f);

		for (int seed = 0; seed < numSeeds; seed++)
		{
			ASSERT_NO_THROW(
				trajectory.RNGProvider.SeedEventStreams(true, seed);
			) << info;

			ASSERT_NO_THROW(
				mdp->InitiateState({ &trajectory,1 });
			) << info << "Did you correctly implement GetInitialState() const or GetInitialState(DynaPlex::RNG&) const";

			if (seed == 0)
			{
				ASSERT_NO_THROW(
					someStates.push_back(trajectory.GetState()->Clone())
				) << info << "Error while cloning state. Does mdp::State support copying?";
			}

			int64_t max_event_count = 128;
			bool finalreached = false;
			int64_t action_count = 0;
			int64_t total_event_count = 0;
			while (trajectory.PeriodCount < max_event_count && !finalreached)
			{
				auto& cat = trajectory.Category;
				if (cat.IsAwaitEvent())
				{
					if (TestEventProbs&& seed==0)
					{
						std::vector<std::tuple<double, DynaPlex::dp_State>> transitions{};
						ASSERT_NO_THROW(
							auto cost = mdp->AllEventTransitions(trajectory.GetState(), transitions);
						) << info << "did you correctly implement GetEventProbs? If you do not intend to implement this on this MDP, do not enable TestEventProbs";
						ASSERT_GE(transitions.size(), 1);
					}
					total_event_count++;
					ASSERT_NO_THROW(
						mdp->IncorporateEvent({ &trajectory,1 });
					) << info << "Did you correctly implement ModifyStateWithEvent  and GetEvent?";
				}
				else if (cat.IsAwaitAction())
				{
					if (ProvidesFlatFeatures)
					{
						ASSERT_NO_THROW(
							mdp->GetFlatFeatures({ &trajectory,1 }, feats_store);
						) << info << "See error: did you correctly implement GetFeatures(State,DynaPlex::Features)?";
						std::vector<float> alt_feats_store(NumFlatFeatures, 0.0f);
						ASSERT_NO_THROW(
							mdp->GetFlatFeatures(trajectory.GetState(), alt_feats_store);
						) << info << "See error: did you correctly implement GetFeatures(State,DynaPlex::Features)?";
						ASSERT_EQ(feats_store, alt_feats_store);
					}
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
					ASSERT_TRUE(!mdp->IsInfiniteHorizon()) << info << " MDP GetStaticInfo indicates that the MDP has infinite horizon, but it returns categories that are IsFinal. Ensure consistency.";
					finalreached = true;
				}
				if (mdp->IsInfiniteHorizon())
				{
					ASSERT_GE(trajectory.PeriodCount * 100 + 100, total_event_count) << info << "A simulation of your trajectory seems incorporate events, but no or hardly any events have index=0. In infinite horizon MDPs, time is accounted for by index=0 events, so ensure that those are present.  ";
				}
				if (!RelaxOnProgramFlow)
				{
					//getting stuck in an action loop should trip this. 
					ASSERT_LE(action_count, max_event_count * 1000) << info << "A simulation of your trajectory seems to get stuck in a loop with only actions, and no events. In ModifyStateWithAction, did you ensure that the state category becomes AwaitEvent or Final? If this is intentional, set RelaxOnLoops to skip this test.";
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
				ASSERT_GE(action_count, trajectory.PeriodCount / 5 - 1) << info << "Substantially less actions then anticipated were taken. Did you ensure that State Category in ModifyStateWithEvent is set to AwaitAction again? Note that PeriodCount is by default only increased when returning StateCategory.Index(0). If your MDP uses hardly any actions intentionally, set RelaxOnLoops to skip this test. ";
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
					trajVec.push_back(std::move(Trajectory(0)));
					
				} 

				trajVec[0].RNGProvider.SeedEventStreams(false, 12, 0);
				trajVec[1].RNGProvider.SeedEventStreams(false, 12, 0);
				mdp->InitiateState({ &trajVec[0] ,1 }, state);
				mdp->InitiateState({ &trajVec[1] ,1 }, loaded_state);

				int64_t max_period_count = 10;
				int64_t action_count = 0;
				
				
				bool finalreached = false;
				while (trajVec[0].PeriodCount < max_period_count && !finalreached)
				{
					auto& cat = trajVec[0].Category;
					ASSERT_EQ(trajVec[0].Category, trajVec[1].Category)<<info<<"Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					ASSERT_EQ(trajVec[0].CumulativeReturn, trajVec[1].CumulativeReturn) << info << "Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					ASSERT_EQ(trajVec[0].PeriodCount, trajVec[1].PeriodCount) << info << "Discrepancy between original state and state after converting to and from VarGroup. MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const and DynaPlex::VarGroup MDP::State::ToVarGroup() const implemented correctly; are all state variables correctly taken into account? Set SkipStateSerializationTests to skip this test. ";
					
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
						ASSERT_TRUE(!mdp->IsInfiniteHorizon()) << info << " MDP GetStaticInfo indicates that the MDP has infinite horizon, but it returns categories that are IsFinal. Ensure consistency.";
						finalreached = true;
					}
					if (!RelaxOnProgramFlow)
					{
						//getting stuck in an action loop should trip this. 
						ASSERT_LE(action_count, max_period_count * 1000) << info << "A simulation of your trajection seems to get stuck in a loop with only actions, and no events. In ModifyStateWithAction, did you ensure that the state category becomes AwaitEvent or Final? If this is intentional, set RelaxOnLoops to skip this test. ";
					}
					else
					{
						if (action_count > 100 * max_period_count)
						{
							finalreached = true;
						}
					}
				}
			}
		}
    }
}