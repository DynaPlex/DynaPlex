#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {	
	TEST(bin_packing, mdp_config_0) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "bin_packing";
		std::string config_name = "mdp_config_0.json";
		ExecuteTest(model_name, config_name);
	}
}