#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/demonstrator.h"
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {
	
	
	TEST(resource_allocation, mdp_config_0) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "resource_allocation";
		std::string config_name = "mdp_config_0.json";
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 16;

		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, config_name);//, policy_config_name);
	}

	TEST(resource_allocation, mdp_config_1) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "resource_allocation";
		std::string config_name = "mdp_config_1.json";
		Tester tester{};
		tester.NumParallelTests = 16;

		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, config_name);
	}
	TEST(resource_allocation, mdp_config_0_policy_0) {
		std::string model_name = "resource_allocation";
		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string mdp_config_name = "mdp_config_1.json";
		//Note: models/model_name/policy_config_name is valid json config file for a policy for "model_name".  
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 16;

		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, mdp_config_name, policy_config_name);
	}


}