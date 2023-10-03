#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/discretedist.h"
using namespace DynaPlex;

int main() {
	try {
		auto& dp = DynaPlexProvider::Get();
		VarGroup config{};
		config.Add("id", "lost_sales");
		config.Add("h", 1.0);
		config.Add("p", 9.0);
		config.Add("leadtime", 5);

		VarGroup dist_config = VarGroup{
			{"type", "poisson"},
			{"mean", 5.0}
		};

		config.Add("demand_dist", dist_config);

		auto mdp = dp.GetMDP(config);
		auto info = mdp->GetStaticInfo();
		VarGroup diagnostics;
		info.Get("diagnostics", diagnostics);
		int64_t MaxSystemInv;
		diagnostics.Get("MaxSystemInv", MaxSystemInv);

		std::vector<DynaPlex::Policy> policies;
		for (int64_t i = 0; i < MaxSystemInv+1; i++)
		{
			VarGroup policy_config{};
			policy_config.Add("id", "base_stock");
			policy_config.Add("base_stock_level", i);
			policies.push_back(mdp->GetPolicy(policy_config));
		}
		//Optional, you can also rely on defaults:
		auto dp_config = VarGroup{
			{"warmup_periods",128},
			{"number_of_trajectories",200},
			{"periods_per_trajectory",1000},
			{"rng_seed",1122}
		};

		int64_t benchmark = -1;
		//To compute all performances relative to the 34th policy. 
		benchmark = 34;

		auto comparer = dp.GetPolicyComparer(mdp,dp_config);
		auto results = comparer.Compare(policies,benchmark);

		for (auto res : results)
		{
			std::cout << res.Dump() << std::endl;
		}

	}
	catch (const DynaPlex::Error& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
