#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
#include "dynaplex/dynaplexprovider.h"
namespace DynaPlex::Tests {	
	TEST(bin_packing, mdp_config_0) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "bin_packing";
		std::string mdp_config_name = "mdp_config_0.json";
		Tester tester{};
		//Opt in for testing the functionality that exposes exact event probabilities. 
		tester.TestEventProbs = true;
		tester.ExecuteTest(model_name, mdp_config_name);
	}



	TEST(bin_packing, assert_features_availability) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "bin_packing";
		std::string mdp_config_name = "mdp_config_0.json";
		Tester tester{};
		//Opt in for testing the functionality that exposes exact event probabilities. 
		tester.TestEventProbs = true;
		tester.AssertFlatFeatureAvailability = true;
		tester.ExecuteTest(model_name, mdp_config_name);
	}

	TEST(bin_packing, trace) {
		std::string model_name = "bin_packing";
		std::string mdp_config_name = "mdp_config_0.json";
		auto& dp = DynaPlexProvider::Get();
		//Get path to IO_DynaPlex/mdp_config_examples/model_name/mdp_config_name:
		std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
		auto mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		auto mdp = dp.GetMDP(mdp_vars_from_json);


		VarGroup vars{ {"max_event_count",10} };
		auto demonstrator = DynaPlexProvider::Get().GetDemonstrator(vars);
		auto trace = demonstrator.GetTrace(mdp);
		for (auto& step : trace)
		{  //uncommenting this will print a trace that enables one to manually verify whether logic is as intended:
			//std::cout << step.Dump() << std::endl;
		}
	}
}