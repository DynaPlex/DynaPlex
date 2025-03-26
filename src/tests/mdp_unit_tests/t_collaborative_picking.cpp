#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/demonstrator.h"
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {
	
	
	/*TEST(collaborative_picking, mdp_config_1) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "collaborative_picking";
		std::string config_name = "mdp_config_1.json";
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 4;
		//note that collaborative_picking states are not equality comparable. 
		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, config_name,policy_config_name);//, policy_config_name);
	}*/
	TEST(collaborative_picking, mdp_config_2) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "collaborative_picking";
		std::string config_name = "mdp_config_2.json";
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 4;
		//note that collaborative_picking states are not equality comparable. 
		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, config_name,policy_config_name);//, policy_config_name);
	}
	/*TEST(collaborative_picking, mdp_config_3) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "collaborative_picking";
		std::string config_name = "mdp_config_3.json";
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 4;
		//note that collaborative_picking states are not equality comparable. 
		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, config_name,policy_config_name);//, policy_config_name);
	}*/
	/*TEST(collaborative_picking, mdp_config_3_1) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "collaborative_picking";
		std::string config_name = "mdp_config_3_1.json";
		std::string policy_config_name = "policy_config_0.json";
		Tester tester{};
		tester.NumParallelTests = 4;
		//note that collaborative_picking states are not equality comparable. 
		tester.SkipEqualityTests = true;
		tester.ExecuteTest(model_name, config_name, policy_config_name);//, policy_config_name);
	}*/

}