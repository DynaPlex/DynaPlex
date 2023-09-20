#pragma once
#include <string>
namespace DynaPlex::Tests {
	/**
	 *Loads json from IORootDir/IO_DynaPlex/defaults/model_name/mdp_config_name
	 *    - configures MDP from that json
	 *If provided, loads json from from IORootDir/IO_DynaPlex/defaults/model_name/agent_config_name
	 *    - configures policy from that json
	 *If not provided, gets random policy.
	 * Performs unit tests using the MDP and the policy. 
	 */
    void ExecuteTest(const std::string& model_name, const std::string& mdp_config_name, const std::string& agent_config_name = "");
}