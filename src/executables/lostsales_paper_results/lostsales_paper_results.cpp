#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;
int main() {

	auto& dp = DynaPlexProvider::Get();

	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"mini_batch_size", 64},
		{"max_training_epochs", 1000},
		{"train_based_on_probs", false}
	};

	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{256,128,128,128}}
	};

	int64_t num_gens=3;
	DynaPlex::VarGroup dcl_config{
		//use paper hyperparameters everywhere. 
		{"N",5000},
		{"num_gens",num_gens},
		{"M",1000},
		{"H", 40},
		{"L", 100},
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"retrain_lastgen_only",false}
	};

	DynaPlex::VarGroup config;
	//retrieve MDP registered under the id string "lost_sales":
	config.Add("id", "lost_sales");
	config.Add("h", 1.0);

	DynaPlex::VarGroup test_config;
	test_config.Add("number_of_trajectories", 100);
	test_config.Add("periods_per_trajectory", 10000);

	std::vector<double> p_values = { 4.0, 9.0, 19.0, 39.0 };
	std::vector<int> leadtime_values = { 2, 3, 4, 6, 8, 10 };
	std::vector<std::string> demand_dist_types = { "poisson", "geometric" };

	size_t num_exp = p_values.size() * leadtime_values.size() * demand_dist_types.size();
	std::vector<DynaPlex::VarGroup> varGroupsMDPs;
	varGroupsMDPs.reserve(num_exp);
	std::vector<std::vector<DynaPlex::VarGroup>> varGroupsPolicies_Mean;
	varGroupsPolicies_Mean.reserve(num_exp);
	std::vector<std::vector<DynaPlex::VarGroup>> varGroupsPolicies_Benchmark;
	varGroupsPolicies_Benchmark.reserve(num_exp);

	for (const std::string& type : demand_dist_types) {
		for (double p : p_values) {
			for (int leadtime : leadtime_values) {
				config.Set("p", p);
				config.Set("leadtime", leadtime);
				config.Set("demand_dist", DynaPlex::VarGroup({
					{"type", type},
					{"mean", 5.0}  
					}));

				DynaPlex::MDP mdp = dp.GetMDP(config);
				auto policy = mdp->GetPolicy("base_stock");
				std::cout << config.Dump() << std::endl;

				// Call and train DCL with specified instance to solve
				auto dcl = dp.GetDCL(mdp, policy, dcl_config);
				dcl.TrainPolicy();

				auto policies = dcl.GetPolicies();
				auto comparer = dp.GetPolicyComparer(mdp, test_config);

				varGroupsMDPs.push_back(config);
				varGroupsPolicies_Mean.push_back(comparer.Compare(policies, 0));
				varGroupsPolicies_Benchmark.push_back(comparer.Compare(policies));
			}
		}	
	}

	for (auto& VarGroup : varGroupsMDPs)
	{
		std::cout << std::endl;
		std::cout << VarGroup.Dump() << std::endl;
		for (auto& VarGroupPolicy : varGroupsPolicies_Mean.front())
		{
			std::cout << VarGroupPolicy.Dump() << std::endl;
		}
		varGroupsPolicies_Mean.erase(varGroupsPolicies_Mean.begin());
		for (auto& VarGroupPolicy : varGroupsPolicies_Benchmark.front())
		{
			std::cout << VarGroupPolicy.Dump() << std::endl;
		}
		varGroupsPolicies_Benchmark.erase(varGroupsPolicies_Benchmark.begin());
		std::cout << std::endl;
	}

	return 0;
}
