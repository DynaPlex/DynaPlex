#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/discretedist.h"
#include <algorithm>
#include <numeric>
using namespace DynaPlex;

/*This iterative procedure works as follows:
* we remove the state-dependeness and only consider a single product in the installed base, which can be fractional AM and CM
* Next we iteratively increase the fractional number of AM items in the installed base, with each fraction, we can calculate the expected demand
* using this expected demand we calculate the optimal* dual index policy, as proposed in Veeraraghavan and Scheller-Wolf (2008) 
* Note that this policy is no longer garantueed to be optimal when used in the MDP, as here the state-dependeness (endogeneous demand) is added again
* 
* We used the implementation of the dual index and single index policy in this Python repo: https://github.com/INFORMSJoC/2022.0136 as an example,
* which is supplied with the paper: "Control of Dual-Sourcing Inventory Systems using Recurrent Neural Networks" by Böttcher, Asikis and Fragkos (2023) IJOC
*/

std::tuple<int64_t,int64_t, int64_t, int64_t> Simulate(std::string policy,DiscreteDist& demandDist, DynaPlex::RNG& rng, int64_t numPeriods, int64_t leadtimeCM, int64_t leadtimeAM, int64_t delta, int64_t target_order_level_am, int64_t target_order_level_cm, double piecePriceAM, double piecePriceCM,double holdingCosts, double backorderCosts)
{
	int64_t current_q_am = 0;
	int64_t current_q_cm = 0;
	std::vector<int64_t> q_am;
	std::vector<int64_t> q_cm;
	for (int64_t i = 0; i <= leadtimeCM; ++i) { q_cm.push_back(0); }
	for (int64_t i = 0; i <= leadtimeAM; ++i) { q_am.push_back(0); }
	int64_t current_inventory_position_am = 0;
	int64_t current_inventory_position_cm = 0;
	std::vector<int64_t> inventory_position_am{ 0 };
	std::vector<int64_t> inventory_position_cm{ 0 };
	int64_t lead_time_difference = leadtimeCM - leadtimeAM;
	int64_t current_inventory = 0;
	double costs = 0.0;
	int64_t	current_demand = 0;

	//single index param
	int64_t current_inventory_position = 0;
	std::vector<int64_t> inventory_position{ 0 };

	for (int64_t t = 0; t < numPeriods; t++)
	{
		//order
		if (policy == "single_index")
		{
			if (current_inventory_position < target_order_level_am)
			{
				q_am.push_back(std::max((int64_t)0, current_demand - delta));
			}
			else
			{
				q_am.push_back(0);
			}
			q_cm.push_back(std::min(delta, current_demand));
		}
		else if (policy == "dual_index")
		{
			q_am.push_back(std::max(0, (int)(target_order_level_am - current_inventory_position_am - q_cm[q_cm.size() - lead_time_difference - 1])));
			q_cm.push_back(std::max(0, (int)(target_order_level_cm - current_inventory_position_cm - q_am[q_am.size() - 1])));
		}
		else
		{
			throw DynaPlex::NotImplementedError();
		}

		//receive shipments
		current_q_am = q_am[q_am.size() - leadtimeAM - 1];
		current_q_cm = q_cm[q_cm.size() - leadtimeCM - 1];

		//reveal demand
		current_demand = demandDist.GetSample(rng);

		//  update inventory and costs
		if (policy == "single_index")
		{
			current_inventory_position -= current_demand;
			current_inventory_position += q_am[q_am.size() - 1];
			current_inventory_position += q_cm[q_cm.size() - 1];
		}
		else if (policy == "dual_index")
		{
			current_inventory_position_am -= current_demand;
			current_inventory_position_am += q_am[q_am.size() - 1];
			current_inventory_position_am += q_cm[lead_time_difference - 1];

			current_inventory_position_cm -= current_demand;
			current_inventory_position_cm += q_am[q_am.size() - 1];
			current_inventory_position_cm += q_cm[q_cm.size() - 1];
		}
		else
		{
			throw DynaPlex::NotImplementedError();
		}

		costs += piecePriceAM * (double)q_am[q_am.size() - 1] + piecePriceCM * (double)q_cm[q_cm.size() - 1] + holdingCosts * std::max(0.0, (double)(current_inventory + current_q_am + current_q_cm - current_demand)) + backorderCosts * std::max(0.0, (double)(current_demand - current_inventory - current_q_am - current_q_cm));

		current_inventory += current_q_am + current_q_cm - current_demand;
		if (policy == "single_index")
		{
			inventory_position.push_back(current_inventory_position);
		}
		else if (policy == "dual_index")
		{
			inventory_position_am.push_back(current_inventory_position_am);
			inventory_position_cm.push_back(current_inventory_position_cm);
		}

	}
	return { current_inventory,costs, std::accumulate(q_am.begin(), q_am.end(), 0), std::accumulate(q_cm.begin(), q_cm.end(), 0) };
}

std::tuple<int64_t, int64_t, double> OptimalIndex(int64_t Instance, std::string policy,DiscreteDist& demandDist, DynaPlex::RNG& rng, DynaPlex::VarGroup mdp_vars_from_json)
{
	std::vector<double> holdingCosts_;
	std::vector<double> backorderCosts_;
	std::vector<int64_t> leadtimeCM_;
	std::vector<int64_t> leadtimeAM_;
	std::vector<double> piecePriceCM_;
	std::vector<double> piecePriceAM_;
	mdp_vars_from_json.Get("holdingCosts", holdingCosts_);
	mdp_vars_from_json.Get("backorderCosts", backorderCosts_);
	mdp_vars_from_json.Get("leadtimeCM", leadtimeCM_);
	mdp_vars_from_json.Get("leadtimeAM", leadtimeAM_);
	mdp_vars_from_json.Get("piecePriceCM", piecePriceCM_);
	mdp_vars_from_json.Get("piecePriceAM", piecePriceAM_);
	double holdingCosts = holdingCosts_[Instance];
	double backorderCosts = backorderCosts_[Instance];
	int64_t leadtimeCM = leadtimeCM_[Instance];
	int64_t leadtimeAM = leadtimeAM_[Instance];
	double piecePriceCM = piecePriceCM_[Instance];
	double piecePriceAM = piecePriceAM_[Instance];
	double critical_fractile = (double)backorderCosts / (double)(backorderCosts + holdingCosts);
	
	std::vector<int64_t> Deltas;
	int64_t numSamples = 1500;//Number of samples to collect to estimate the convolved demand and overshoot distribution
	int64_t numPeriods = 700;//Number of periods to simulate for each sample
	int64_t z_am = 0;//dual index param
	int64_t z_cm = 0;//single index param
	for (int64_t i = -10; i <= 10; ++i) { Deltas.push_back(i); }//may seem  weird, but you can consider to include negative delta (just to be sure)
	std::vector<int64_t> target_order_arr;		

	for (auto& delta: Deltas)
	{
		int64_t target_order_level_cm = 0;
		int64_t target_order_level_am = 0;

		if (policy == "single_index")
		{
			target_order_level_cm = z_cm;
			target_order_level_am = z_cm - delta;
		}
		else if (policy == "dual_index")
		{
			target_order_level_cm = z_am + delta;
			target_order_level_am = z_am;
		}

		int64_t current_inventory = z_am;
		std::vector<int64_t> index_G_Delta;

		for (int64_t i = 0; i < numSamples; i++)
		{
			auto [current_inventory, costs, q_am, q_cm] = Simulate(policy, demandDist, rng, numPeriods, leadtimeCM, leadtimeAM, delta, target_order_level_am, target_order_level_cm, piecePriceAM, piecePriceCM, holdingCosts, backorderCosts);
			if (policy == "single_index") { index_G_Delta.push_back(target_order_level_cm - current_inventory); }
			else if (policy == "dual_index") { index_G_Delta.push_back(target_order_level_am - current_inventory); }
		}
		//sort vectors and obtain
		std::sort(index_G_Delta.begin(), index_G_Delta.end());
		int64_t z_ = index_G_Delta[(int)(index_G_Delta.size() * critical_fractile)];
		target_order_arr.push_back(z_);

	}

	std::vector<int64_t> cost_per_setting;
	std::vector<double> am_ratio;
	for (int64_t i = 0; i < Deltas.size(); i++)
	{
		int64_t total_costs = 0;
		int64_t total_q_am = 0;
		int64_t total_q_cm = 0;

		int64_t target_order_level_cm = 0;
		int64_t target_order_level_am = 0;
		if (policy == "single_index")
		{
			target_order_level_cm = target_order_arr[i];
			target_order_level_am = target_order_arr[i] - Deltas[i];
		}
		else if (policy == "dual_index")
		{
			target_order_level_cm = target_order_arr[i] + Deltas[i];
			target_order_level_am = target_order_arr[i];
		}
		
		for (int64_t n = 0; n < numSamples; n++)//could be done in parallel
		{
			auto [current_inventory,costs, q_am,q_cm] = Simulate(policy,demandDist, rng, numPeriods, leadtimeCM, leadtimeAM, Deltas[i], target_order_level_am, target_order_level_cm, piecePriceAM, piecePriceCM, holdingCosts, backorderCosts);
			total_costs += costs;
			total_q_am += q_am;
			total_q_cm += q_cm;
		}
		cost_per_setting.push_back(total_costs);
		am_ratio.push_back(std::max(0.0,(double)total_q_am / (double)(total_q_am+ total_q_cm)));
	}
	
	int64_t index = std::distance(cost_per_setting.begin(), std::min_element(cost_per_setting.begin(), cost_per_setting.end()));
	
	//return the optimal Delta and z
	return { Deltas[index],target_order_arr[index],am_ratio[index]};

}

int64_t Fact(int64_t num)
{
	if (num > 1)
	{
		return num * Fact(num - 1);
	}
	else
	{
		return 1;
	}
}

std::vector<double> GetNegBinPMF(long uBound, double r, double p)
{
	if (p < 0 || p> 1.0 || r < 0.0 || uBound < 0)
	{
		std::cout << "wrong Neg Bin param";
	}
	std::vector<double> returnVec;
	long i = 0;
	returnVec.reserve(uBound);
	double sum = 0.0;
	while (i < uBound)
	{
		double factor = (std::tgamma(r + (double)i) / (Fact(i) * std::tgamma(r))) * std::pow(p, r) * std::pow(1.0 - p, (double)i);
		if (factor < 0.0)
		{
			return returnVec;
		}
		sum += factor;
		returnVec.push_back(factor);
		i++;
	}
	if (uBound > 0)
	{
		returnVec.push_back(std::max(1.0 - sum, 0.0));
	}
	else
	{
		returnVec.push_back(1.0);
	}

	return returnVec;

}

std::vector<double> GetPoissonPMF(long uBound, double rate)
{//rate<100
	std::vector<double> returnVec;

	returnVec.reserve(uBound);
	long i = 0;
	double factor = std::exp(-rate);
	double sum = 0.0;
	while (i < uBound)
	{
		if (factor < 0.0)
		{
			return returnVec;
		}
		sum += factor;
		returnVec.push_back(factor);
		factor *= rate / (++i);
	}
	if (uBound > 0)
	{
		if ((1.0 - sum) < 0.0)
		{
			returnVec.push_back(0.0);
		}
		else
		{
			returnVec.push_back(1.0 - sum);
		}

	}
	else
	{
		returnVec.push_back(1.0);
	}


	return returnVec;

}

int main() {
	try {
		auto& dp = DynaPlexProvider::Get();
		std::string model_name = "dual_sourcing";
		std::string mdp_config_name = "mdp_configs/mdp_config_0.json";
		std::string policy = "dual_index";// "dual_index"
		std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);

		std::vector<int64_t> placeholder_delta = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		std::vector<int64_t> placeholder_orderupto_am = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		for (int64_t inst = 0; inst < 10; inst++)
		{
			auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
			mdp_vars_from_json.Add("inst_no", inst);
			auto mdp = dp.GetMDP(mdp_vars_from_json);

			double delta = 1.0;
			double am_cm_fraction = 0.15;

			int64_t Instance = inst;//ensure that in mdp.cpp the instance is not randomly drawn but the same as this one, see GetInitialState()!

			std::vector<double> meanFailureAM_;
			std::vector<double> meanFailureCM_;
			std::vector<double> varFailureAM_;
			std::vector<double> varFailureCM_;
			std::vector<int64_t> InstalledBase_;
			mdp_vars_from_json.Get("meanFailureAM", meanFailureAM_);
			mdp_vars_from_json.Get("meanFailureCM", meanFailureCM_);
			mdp_vars_from_json.Get("varFailureAM", varFailureAM_);
			mdp_vars_from_json.Get("varFailureCM", varFailureCM_);
			mdp_vars_from_json.Get("InstalledBase", InstalledBase_);
			double meanFailureAM = meanFailureAM_[Instance];
			double meanFailureCM = meanFailureCM_[Instance];
			double varFailureAM = varFailureAM_[Instance];
			double varFailureCM = varFailureCM_[Instance];
			int64_t InstalledBase = InstalledBase_[Instance];
			//std::cout << "Check if runnign with correct dist!!!" << std::endl;

			std::cout << inst << std::endl;

			DynaPlex::RNG rng{true, (int64_t)78234984 };//for demand gen

			int64_t counter = 0;//fail safe counter
			int64_t opt_z_;
			int64_t opt_delta;
			int64_t diff;
			while (delta > 0.001 && counter < 15)
			{
				double expected_demand = am_cm_fraction * (InstalledBase * meanFailureAM) + (1 - am_cm_fraction) * (InstalledBase * meanFailureCM);
				double var_demand = am_cm_fraction * varFailureAM + (1 - am_cm_fraction) * varFailureCM + am_cm_fraction * (1 - am_cm_fraction) * std::pow(meanFailureAM - meanFailureCM, 2);

				//if (var_demand < expected_demand)
				//{
				//	//not supported at the momemt, requires Poisson dist.
				//	throw DynaPlex::NotImplementedError("Var demand smaller than mean not supported (yet)");
				//}

				double p = 1.0 - (expected_demand / var_demand);
				double r = expected_demand * ((1.0 - p) / p);


				//auto demandDist = DiscreteDist::GetCustomDist(GetNegBinPMF((long)InstalledBase, r, 1.0 - p));

				auto demandDist = DiscreteDist::GetCustomDist(GetPoissonPMF((long)InstalledBase, expected_demand));

				auto [delta_index, z_, am_ratio] = OptimalIndex(Instance, policy, demandDist, rng, mdp_vars_from_json);

				double new_am_cm_fraction = am_ratio * meanFailureCM / (((1 - am_ratio) * meanFailureAM) + (am_ratio * meanFailureCM));
				delta = std::abs(new_am_cm_fraction - am_cm_fraction);
				am_cm_fraction = new_am_cm_fraction;
				counter++;
				if (delta <= 0.001 || counter == 15)
				{
					opt_z_ = z_;
					opt_delta = delta_index;
					std::cout << policy << std::endl;
					std::cout << "optimal delta: " << delta_index << ", optimal z: " << z_ << ", am_ratio: " << am_ratio << ", am/cm fraction: " << am_cm_fraction << ", iterations: " << counter << std::endl;
					if (z_<0)
					{
						diff = delta_index + z_;
						std::cout << "diff:" << diff << std::endl;
					}
					else
					{
						diff = delta_index - z_;
						std::cout << "diff:" << diff << std::endl;
					}
					
					
				}
			}

			std::vector<DynaPlex::Policy> policies;
			VarGroup policy_config{};
			policy_config.Add("id", policy);
			if (policy == "single_index")
			{
				placeholder_orderupto_am[inst] = opt_z_;
				policy_config.Add("orderupto_cm", placeholder_orderupto_am);
			}
			else
			{
				placeholder_orderupto_am[inst] =  opt_z_;
				policy_config.Add("orderupto_am_", placeholder_orderupto_am);
			}
			placeholder_delta[inst] = opt_delta;
			policy_config.Add("delta_", placeholder_delta);
			policies.push_back(mdp->GetPolicy(policy_config));

		//};

			//we do a check in the neighborhood of the optimal values to validate 
			//for (int64_t i = -20; i < 20; i++)
			//{
			//	VarGroup policy_config{};
			//	policy_config.Add("id", policy);
			//	if (policy == "single_index")
			//	{
			//		placeholder_orderupto_am[inst] = i;
			//		policy_config.Add("orderupto_cm", placeholder_orderupto_am);
			//	}
			//	else
			//	{
			//		placeholder_orderupto_am[inst] = i;
			//		policy_config.Add("orderupto_am_", placeholder_orderupto_am);
			//	}
			//	placeholder_delta[inst] = diff - i;
			//	policy_config.Add("delta_", placeholder_delta);
			//	policies.push_back(mdp->GetPolicy(policy_config));

			//}

			for (int64_t bsp = 0; bsp < 10; bsp++)
			{
				VarGroup policy_config{};
				policy_config.Add("id", "base_stock");
				policy_config.Add("orderupto_cm", bsp);
				policies.push_back(mdp->GetPolicy(policy_config));
			}

			//Optional, you can also rely on defaults:
			auto dp_config = VarGroup{
				{"warmup_periods",100},
				{"number_of_trajectories",500},
				{"periods_per_trajectory",850},
				{"rng_seed",11822}
			};

			auto comparer = dp.GetPolicyComparer(mdp, dp_config);
			auto results = comparer.Compare(policies, 0);

			for (auto res : results)
			{
				std::cout << res.Dump() << std::endl;
			}
		};

	}
	catch (const DynaPlex::Error& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}

