#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;
int main() {

	auto& dp = DynaPlexProvider::Get();

	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"mini_batch_size", 64},
		{"max_training_epochs", 1000}
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
	config.Add("number_of_trajectories", 100);
	config.Add("periods_per_trajectory", 10000);

	std::vector<double> p_values = { 4.0, 9.0, 19.0, 39.0 };
	std::vector<int> leadtime_values = { 2, 3, 4, 6, 8, 10 };
	std::vector<std::string> demand_dist_types = { "poisson", "geometric" };

	for (double p : p_values) {
		for (int leadtime : leadtime_values) {
			for (const std::string& type : demand_dist_types) {
				config.Set("p", p);
				config.Set("leadtime", leadtime);
				config.Set("demand_dist", DynaPlex::VarGroup({
					{"type", type},
					{"mean", 5.0}  
					}));

				DynaPlex::MDP mdp = dp.GetMDP(config);
				auto policy = mdp->GetPolicy("base_stock");

				// Call and train DCL with specified instance to solve
				auto dcl = dp.GetDCL(mdp, dcl_config, policy);
				dcl.TrainPolicy();

				auto policies = dcl.GetPolicies();
				auto comparer = dp.GetPolicyComparer(mdp, config);
				auto relative_comparison = comparer.Compare(policies, 0);
				for (auto& VarGroup : relative_comparison)
				{
					std::cout << VarGroup.Dump() << std::endl;
				}
				auto mean_comparison = comparer.Compare(policies);
				for (auto& VarGroup : mean_comparison)
				{
					std::cout << VarGroup.Dump() << std::endl;
				}
			}
		}	
	}

	return 0;
}
