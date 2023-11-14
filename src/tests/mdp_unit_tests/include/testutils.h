#pragma once
#include <string>
namespace DynaPlex::Tests {

	class Tester
	{
	public:
		/**
		 *Loads json from IORootDir/IO_DynaPlex/defaults/model_name/mdp_config_name
		 *    - configures MDP from that json
		 *If argument policy_config_name provided,
		 *    - loads json from from IORootDir/IO_DynaPlex/defaults/model_name/policy_config_name
		 *    - configures policy from that json
		 *If argument policy_config_name not provided, gets random policy.
		 *Performs range of tests using the MDP and the Policy.
		 */
		void ExecuteTest(const std::string& model_name, const std::string& mdp_config_name, const std::string& policy_config_name = "");

		Tester() {};
		/// if set to true, test asserts that the MDP correctly implements flat features. 
		bool AssertFlatFeatureAvailability = false;
		/*
		 *Typical MDPs will alternate between consuming events and consuming actions, e.g. each time one action and one event. Some tests are conducted
		 * to check whether the MDP has a reasonable number of actions and events. If you MDP has only events, or only actions, you can set relaxonloops to skip these tests. 
		 */ 		
		bool RelaxOnProgramFlow = false;
		/// Skip tests related to state equality, when MDP does not support testing states for equality. 
		bool SkipEqualityTests = false;
		/// Skip tests related to serialization of states, e.g. when the MDP does not support State GetState(const VarGroup&) const;
		bool SkipStateSerializationTests = false;
		/// If tests are slow, you can bring this number down.  
		int64_t NumParallelTests = 64;
	};
}