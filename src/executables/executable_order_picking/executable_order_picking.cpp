#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;

void compareAgents();

int main() {

	compareAgents();
    return 0;
}

void compareAgents() {

	std::string model_name = "order_picking";
	std::string mdp_config_name = "mdp_config_0.json";
	std::string policy_config_name = "policy_config_0.json";
	auto& dp = DynaPlexProvider::Get();

	//Get path to IO_DynaPlex/mdp_config_examples/model_name/mdp_config_name:
	std::string mdp_file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
	auto mdp_vars_from_json = VarGroup::LoadFromFile(mdp_file_path);
	auto mdp = dp.GetMDP(mdp_vars_from_json);

	VarGroup policy_vars_from_json;
	std::string policy_file_path = dp.System().filepath("mdp_config_examples", model_name, policy_config_name);
	policy_vars_from_json = VarGroup::LoadFromFile(policy_file_path);

	DynaPlex::Policy random_policy = mdp->GetPolicy("random");
	DynaPlex::Policy heuristic_policy = mdp->GetPolicy(policy_vars_from_json);

	VarGroup comparerConfig;
	comparerConfig.Add("number_of_trajectories", 100);
	comparerConfig.Add("periods_per_trajectory", 100);

	auto comparer = dp.GetPolicyComparer(mdp, comparerConfig);

	auto comparison = comparer.Compare({ random_policy, heuristic_policy });
	for (auto& VarGroup : comparison)
	{
		std::cout << VarGroup.Dump() << std::endl;
	}
}