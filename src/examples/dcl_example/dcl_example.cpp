#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;
int main() {

	auto& dp = DynaPlexProvider::Get();


	DynaPlex::VarGroup config;
	//retrieve MDP registered under the id string "lost_sales":
	config.Add("id", "lost_sales");
	//add other parameters required by that MDP:
	config.Add("p", 9.0);
	config.Add("h", 1.0);
	config.Add("leadtime", 4);
	config.Add("demand_dist", 
		DynaPlex::VarGroup({
		{"type", "poisson"},
		{"mean", 5.0}
		}));

	DynaPlex::MDP mdp = dp.GetMDP(config);

	config.Set("p", 90.0);
	config.Set("h", 10.0);
	//for illustration purposes, create a different mdp 
	//that is compatible with the first - same number of features, same number of valid actions:
	DynaPlex::MDP different_mdp = dp.GetMDP(config);

	auto policy = mdp->GetPolicy("base_stock");

	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",5}
	};

	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{64,64}}
	};
	int64_t num_gens=1;
	DynaPlex::VarGroup dcl_config{
		//use defaults everywhere. 
		{"N",5000},
		{"num_gens",num_gens},
		{"M",400},
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"retrain_lastgen_only",false}
	};

	try
	{
		//Create a trainer for the mdp, with appropriate configuratoin. 
		auto dcl = dp.GetDCL(mdp, dcl_config, policy);
		//this trains the policy, and saves it to disk.
		dcl.TrainPolicy();
		//using a dcl instance that has same parameterization (i.e. same dcl_config, same mdp), we may recover the trained polciies.
		//This gets the policy that was trained last:
		//auto policy = dcl.GetPolicy();
		//This gets policy with specific index:
		//auto first = dcl.GetPolicy(1);
		
		//This gets all trained policy, as well as the initial policy, in a vector:
		auto policies = dcl.GetPolicies();




		//Compare the various trained policies:
		auto comparer = dp.GetPolicyComparer(mdp);
		auto comparison = comparer.Compare(policies);
		for (auto& VarGroup : comparison)
		{
			std::cout << VarGroup.Dump() << std::endl;
		}


		//policies are automatically saved when training, but it may be usefull to save at custom location:
		auto last_policy = dcl.GetPolicy();
		//gets a file_path without file extension (file extensions are automatically added when saving): 
		auto path = dp.System().filepath("dcl", "dcl_example", "lost_sales_policy_gen"+ num_gens);
		//this is IOLocation/dcl/dcl_example/lost_sales_policy
		//IOLocation is typically specified in CMakeUserPresets.txt

		//saves two files, one .json file with the architecture (e.g. trained_lost_sales_policy.json), and another file with neural network weights (.pth):		
		dp.SavePolicy(last_policy, path);
		
		//This loads the policy again from the same path, automatically adding the right extensions:
		auto policy = dp.LoadPolicy(mdp, path);
		
		//Even possible to load the policy trained for one MDP, and make it applicable to another mdp:
		//this however only works if the two policies have consistent input and output dimensions, i.e.
		//same number of valid actions and same number of features. 
		auto different_policy = dp.LoadPolicy(different_mdp, path);
	}
	catch (const std::exception& e)
	{
		std::cout << "exception: " << e.what() << std::endl;
	}
	return 0;
}
