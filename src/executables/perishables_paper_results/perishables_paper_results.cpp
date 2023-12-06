#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;

//Find the best base-stock level
int64_t FindBestBSLevel(DynaPlex::VarGroup& config)
{
	auto& dp = DynaPlexProvider::Get();
	DynaPlex::MDP mdp = dp.GetMDP(config);

	DynaPlex::VarGroup test_config;
	test_config.Add("number_of_trajectories", 100);
	test_config.Add("periods_per_trajectory", 10000);
	test_config.Add("rng_seed", 1122);
	auto comparer = dp.GetPolicyComparer(mdp, test_config);

	double bestBScost = std::numeric_limits<double>::infinity();
	int64_t BSLevel = 1;
	int64_t bestBSLevel = BSLevel;

	DynaPlex::VarGroup policy_config;
	policy_config.Add("id", "base_stock");
	policy_config.Add("base_stock_level", BSLevel);

	while (true)
	{
		auto policy = mdp->GetPolicy(policy_config);
		auto comparison = comparer.Assess(policy);
		double cost;
		comparison.Get("mean", cost);
		if (cost < bestBScost)
		{
			bestBScost = cost;
			bestBSLevel = BSLevel;
			BSLevel++;
			policy_config.Set("base_stock_level", BSLevel);
		}
		else {
			break;
		}
	}

	return bestBSLevel;
}

void DCL_Test()
{
	auto& dp = DynaPlexProvider::Get();

	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"mini_batch_size", 64},
		{"max_training_epochs", 1000},
		{"train_based_on_probs", false}
	};

	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{64,64}}
	};

	int64_t num_gens = 1;
	DynaPlex::VarGroup dcl_config{
		//use paper hyperparameters everywhere. 
		{"N",500},
		{"num_gens",num_gens},
		{"M",100},
		{"H", 40},
		{"L", 100},
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"retrain_lastgen_only",false},
		{"enable_sequential_halving", true }
	};

	DynaPlex::VarGroup config;
	//retrieve MDP registered under the id string "lost_sales":
	config.Add("id", "perishable_systems");
	config.Add("o", 100.0);
	config.Add("h", 0.0);
	config.Add("c", 0.0);
	config.Add("costratio", 1.0);
	config.Add("f", 1.0);
	config.Add("mu", 4.0);
	config.Add("cvr", 1.0);
	config.Add("ProductLife", 3);
	config.Add("LeadTime", 0);
	config.Add("discount_factor", 1.0);

	DynaPlex::MDP mdp = dp.GetMDP(config);
	auto policy = mdp->GetPolicy("base_stock");

	DynaPlex::VarGroup test_config;
	test_config.Add("number_of_trajectories", 100);
	test_config.Add("periods_per_trajectory", 1000);


	auto dcl = dp.GetDCL(mdp, policy, dcl_config);
	//this trains the policy, and saves it to disk.
	dcl.TrainPolicy();
	auto policies = dcl.GetPolicies();

	int64_t BestBSLevel = FindBestBSLevel(config);
	DynaPlex::VarGroup policy_config;
	policy_config.Add("id", "base_stock");
	policy_config.Add("base_stock_level", BestBSLevel);
	auto best_bs_policy = mdp->GetPolicy(policy_config);
	policies.push_back(best_bs_policy);
	auto comparer = dp.GetPolicyComparer(mdp, test_config);
	auto comparison = comparer.Compare(policies);

	double best_nn_cost = std::numeric_limits<double>::infinity();
	double best_bs_cost{ 0.0 };
	for (auto& VarGroup : comparison)
	{
		std::cout << VarGroup.Dump() << std::endl;
		DynaPlex::VarGroup policy_id;
		VarGroup.Get("policy", policy_id);
		std::string id;
		policy_id.Get("id", id);

		if (id == "NN_Policy") {
			double nn_cost;
			VarGroup.Get("mean", nn_cost);
			if (nn_cost < best_nn_cost) {
				best_nn_cost = nn_cost;
			}
		}
		else if (id == "base_stock") {
			int64_t BS_level;
			if (policy_id.HasKey("base_stock_level")) {
				policy_id.Get("base_stock_level", BS_level);
				if (BS_level == BestBSLevel) {
					VarGroup.Get("mean", best_bs_cost);
				}
			}
		}
	}
	std::cout << std::endl;
	std::cout << "Best base-stock policy cost:  " << best_bs_cost << "  best nn-policy cost:  " << best_nn_cost << std::endl;
	std::cout << std::endl;
}

// Test of Temizoz et al. (2023) instances
void TestPaperInstances()
{
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

	DynaPlex::VarGroup dcl_config{
		//use paper hyperparameters everywhere. 
		{"N",5000},
		{"num_gens",3},
		{"M",1000},
		{"H", 40},
		{"L", 100},
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training}
	};

	DynaPlex::VarGroup test_config;
	test_config.Add("number_of_trajectories", 100);
	test_config.Add("periods_per_trajectory", 10000);

	std::vector<std::vector<std::vector<double>>> ShelfLifeResults(3);
	std::vector<std::vector<std::vector<double>>> LeadTimeResults(3);
	std::vector<std::vector<std::vector<double>>> CVRResults(3);
	std::vector<std::vector<std::vector<double>>> fResults(3);
	std::vector<std::vector<double>> AllResults;

	int64_t ShelfLifeIndex = 0;
	for (int64_t m : {3, 4, 5})
	{
		int64_t LeadTimeIndex = 0;
		for (int64_t leadtime : {0, 1, 2})
		{
			int64_t CVRindex = 0;
			for (double cvr : { 1.0, 1.5, 2.0 })
			{
				int64_t findex = 0;
				for (double f : {1.0, 0.5, 0.0})
				{
					DynaPlex::VarGroup config;
					config.Add("id", "perishable_systems");
					config.Add("f", f);
					config.Add("cvr", cvr);
					config.Add("ProductLife", m);
					config.Add("LeadTime", leadtime);

					DynaPlex::MDP mdp = dp.GetMDP(config);
					auto policy = mdp->GetPolicy("base_stock");
					auto dcl = dp.GetDCL(mdp, policy, dcl_config);
					dcl.TrainPolicy();
					auto policies = dcl.GetPolicies();

					int64_t BestBSLevel = FindBestBSLevel(config);
					DynaPlex::VarGroup policy_config;
					policy_config.Add("id", "base_stock");
					policy_config.Add("base_stock_level", BestBSLevel);
					auto best_bs_policy = mdp->GetPolicy(policy_config);
					policies.push_back(best_bs_policy);

					auto comparer = dp.GetPolicyComparer(mdp, test_config);
					auto comparison = comparer.Compare(policies);

					std::cout << std::endl;
					std::cout << config.Dump() << std::endl;

					double best_nn_cost = std::numeric_limits<double>::infinity();
					double best_bs_cost{ 0.0 };
					for (auto& VarGroup : comparison)
					{
						std::cout << VarGroup.Dump() << std::endl;
						DynaPlex::VarGroup policy_id;
						VarGroup.Get("policy", policy_id);
						std::string id;
						policy_id.Get("id", id);

						if (id == "NN_Policy") {
							double nn_cost;
							VarGroup.Get("mean", nn_cost);
							if (nn_cost < best_nn_cost) {
								best_nn_cost = nn_cost;
							}
						}
						else if (id == "base_stock") {
							int64_t BS_level;
							if (policy_id.HasKey("base_stock_level")) {
								policy_id.Get("base_stock_level", BS_level);
								if (BS_level == BestBSLevel) {
									VarGroup.Get("mean", best_bs_cost);
								}
							}
						}
					}

					double BSNNGap = (100 * (best_nn_cost - best_bs_cost) / best_bs_cost);
					std::cout << std::endl;
					std::cout << "Best base-stock policy cost:  " << best_bs_cost << "  best nn-policy cost:  " << best_nn_cost << "  gap:  " << BSNNGap << std::endl;
					std::cout << std::endl;

					std::vector<double> results{};
					results.push_back(best_bs_cost);
					results.push_back(best_nn_cost);
					results.push_back(BSNNGap);

					ShelfLifeResults[ShelfLifeIndex].push_back(results);
					LeadTimeResults[LeadTimeIndex].push_back(results);
					CVRResults[CVRindex].push_back(results);
					fResults[findex].push_back(results);
					AllResults.push_back(results);

					findex++;
				}
				CVRindex++;
			}
			LeadTimeIndex++;
		}
		ShelfLifeIndex++;
	}

	std::cout << std::endl;
	//All results
	double BSCosts{ 0.0 };
	double NNCosts{ 0.0 };
	double BSNNGaps{ 0.0 };

	for (size_t i = 0; i < AllResults.size(); i++)
	{
		BSCosts += AllResults[i][0];
		NNCosts += AllResults[i][1];
		BSNNGaps += AllResults[i][2];
	}
	size_t TotalNumInstance = AllResults.size();
	std::cout << "Avg BS Costs:  " << BSCosts / TotalNumInstance << "  , Avg NN Costs:  " << NNCosts / TotalNumInstance;
	std::cout << "  , Avg BS - NN Gap:  " << BSNNGaps / TotalNumInstance << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	//Shelf life results
	size_t mcount = 0;
	for (size_t m : {3, 4, 5})
	{
		double mBSCosts{ 0.0 };
		double mNNCosts{ 0.0 };
		double mBSNNGaps{ 0.0 };

		for (size_t i = 0; i < ShelfLifeResults[mcount].size(); i++)
		{
			mBSCosts += ShelfLifeResults[mcount][i][0];
			mNNCosts += ShelfLifeResults[mcount][i][1];
			mBSNNGaps += ShelfLifeResults[mcount][i][2];
		}
		size_t mTotalNumInstance = ShelfLifeResults[mcount].size();
		std::cout << "Product life:  " << m << "  Avg BS Costs:  " << mBSCosts / mTotalNumInstance;
		std::cout << "  , Avg NN Costs:  " << mNNCosts / mTotalNumInstance;
		std::cout << "  , Avg BS - NN Gap:  " << mBSNNGaps / mTotalNumInstance << std::endl;

		mcount++;
	}
	std::cout << std::endl;
	std::cout << std::endl;

	//Lead time results
	size_t lcount = 0;
	for (size_t l : {0, 1, 2})
	{
		double mBSCosts{ 0.0 };
		double mNNCosts{ 0.0 };
		double mBSNNGaps{ 0.0 };

		for (size_t i = 0; i < LeadTimeResults[lcount].size(); i++)
		{
			mBSCosts += LeadTimeResults[lcount][i][0];
			mNNCosts += LeadTimeResults[lcount][i][1];
			mBSNNGaps += LeadTimeResults[lcount][i][2];
		}
		size_t mTotalNumInstance = LeadTimeResults[lcount].size();
		std::cout << "Lead time:  " << l << "  Avg BS Costs:  " << mBSCosts / mTotalNumInstance;
		std::cout << "  , Avg NN Costs:  " << mNNCosts / mTotalNumInstance;
		std::cout << "  , Avg BS - NN Gap:  " << mBSNNGaps / mTotalNumInstance << std::endl;

		lcount++;
	}
	std::cout << std::endl;
	std::cout << std::endl;

	//CVR results
	size_t ccount = 0;
	for (double cvr : {1.0, 1.5, 2.0})
	{
		double mBSCosts{ 0.0 };
		double mNNCosts{ 0.0 };
		double mBSNNGaps{ 0.0 };

		for (size_t i = 0; i < CVRResults[ccount].size(); i++)
		{
			mBSCosts += CVRResults[ccount][i][0];
			mNNCosts += CVRResults[ccount][i][1];
			mBSNNGaps += CVRResults[ccount][i][2];
		}
		size_t mTotalNumInstance = CVRResults[ccount].size();
		std::cout << "CVR:  " << cvr << "  Avg BS Costs:  " << mBSCosts / mTotalNumInstance;
		std::cout << "  , Avg NN Costs:  " << mNNCosts / mTotalNumInstance;
		std::cout << "  , Avg BS - NN Gap:  " << mBSNNGaps / mTotalNumInstance << std::endl;

		ccount++;
	}
	std::cout << std::endl;
	std::cout << std::endl;

	//f results
	size_t fcount = 0;
	for (double f : {1.0, 0.5, 0.0})
	{
		double mBSCosts{ 0.0 };
		double mNNCosts{ 0.0 };
		double mBSNNGaps{ 0.0 };

		for (size_t i = 0; i < fResults[fcount].size(); i++)
		{
			mBSCosts += fResults[fcount][i][0];
			mNNCosts += fResults[fcount][i][1];
			mBSNNGaps += fResults[fcount][i][2];
		}
		size_t mTotalNumInstance = fResults[fcount].size();
		std::cout << "Issuance Policy f:  " << f << "  Avg BS Costs:  " << mBSCosts / mTotalNumInstance;
		std::cout << "  , Avg NN Costs:  " << mNNCosts / mTotalNumInstance;
		std::cout << "  , Avg BS - NN Gap:  " << mBSNNGaps / mTotalNumInstance << std::endl;

		fcount++;
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

int main() {

	//DCL_Test();
	TestPaperInstances();

	return 0;

}