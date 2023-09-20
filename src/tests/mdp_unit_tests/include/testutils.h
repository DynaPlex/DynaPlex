#pragma once
#include <string>
namespace DynaPlex::Tests {
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
}