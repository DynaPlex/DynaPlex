#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
#include "dynaplex/dynaplexprovider.h"

namespace DynaPlex::Tests {
	TEST(order_picking, mdp_config_0_random) {
		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "order_picking"; // this should match the id and namespace name discussed earlier
		std::string mdp_config_name = "mdp_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 4;
		tester.ExecuteTest(model_name, mdp_config_name);
	}

	TEST(order_picking, mdp_config_0_heuristic) {
		std::string model_name = "order_picking";
		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string mdp_config_name = "mdp_config_0.json";
		//Note: models/model_name/policy_config_name is valid json config file for a policy for "model_name".  
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		
		tester.NumParallelTests = 4;
		
		//tester.ExecuteTest(model_name, mdp_config_name);
		tester.ExecuteTest(model_name, mdp_config_name, policy_config_name);
	}
}