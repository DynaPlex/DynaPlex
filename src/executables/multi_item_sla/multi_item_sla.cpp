#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/modelling/discretedist.h"
#include <algorithm>

using namespace DynaPlex;

class InitialMDPProcessing
{
public:
	DynaPlex::VarGroup class_config;

	int64_t benchmarkAction;
	int64_t incumbentAction;
	double best_static_cost;
	double best_static_afr;
	int64_t greedyAction;
	int64_t totalActions;
	int64_t simulationCount;
	double incumbentTarget;
	std::vector<int64_t> compositeActions;

private:
	DynaPlex::VarGroup test_config;

	int64_t numberOfItems;
	double aggregateTargetFillRate;
	int64_t reviewHorizon;
	double penaltyCost;

	std::vector<double> holdingCosts;
	std::vector<int64_t> leadTimes;
	std::vector<double> demandRates;
	std::vector<int64_t> highDemandVariance;

	double totalDemandRate;
	std::vector<DiscreteDist> demand_distributions;
	std::vector<DiscreteDist> demand_distributions_over_leadtime;
	double unavoidableCostPerPeriod = 0.0;

	std::vector<std::vector<int64_t>> allBaseStockLevels;
	std::vector<double> allExpectedPolicyHoldingCosts;
	std::vector<double> allExpectedPolicyFillRates;
	std::vector<double> allExpectedPolicyStockoutAmounts;

	std::vector<int64_t> setL;
	std::vector<int64_t> setU;

public:

	InitialMDPProcessing(DynaPlex::VarGroup& config, DynaPlex::VarGroup test_config) : class_config{ config }, test_config{ test_config }
	{
		auto& dp = DynaPlexProvider::Get();

		int64_t leadTime;
		double demand_rate_limit;
		double high_variance_ratio;
		double cost_limit;
		int64_t seed;
		class_config.Get("numberOfItems", numberOfItems);
		class_config.Get("leadTime", leadTime);
		class_config.Get("demand_rate_limit", demand_rate_limit);
		class_config.Get("high_variance_ratio", high_variance_ratio);
		class_config.Get("cost_limit", cost_limit);
		class_config.GetOrDefault("seed", seed, 10061994 + 4081965); //lucky birthdays

		DynaPlex::RNG rng(true, seed);

		holdingCosts.reserve(numberOfItems);
		demandRates.reserve(numberOfItems);
		leadTimes.reserve(numberOfItems);
		highDemandVariance.reserve(numberOfItems);
		demand_distributions.reserve(numberOfItems);
		demand_distributions_over_leadtime.reserve(numberOfItems);
		std::vector<int64_t> stockLevels;
		stockLevels.reserve(numberOfItems);
		totalDemandRate = 0.0;
		for (int64_t i = 0; i < numberOfItems; i++)
		{
			holdingCosts.push_back(rng.genUniform() * cost_limit);
			demandRates.push_back(rng.genUniform() * demand_rate_limit);
			leadTimes.push_back(leadTime);
			if (rng.genUniform() > high_variance_ratio) {
				highDemandVariance.push_back(0);
				demand_distributions.push_back(DiscreteDist::GetPoissonDist(demandRates[i]));
			}
			else {
				highDemandVariance.push_back(1);
				demand_distributions.push_back(DiscreteDist::GetGeometricDist(demandRates[i]));
			}

			if (leadTimes[i] > 0) {
				auto DemOverLeadtime = DiscreteDist::GetZeroDist();
				for (int64_t k = 0; k < leadTimes[i]; k++) {
					DemOverLeadtime = DemOverLeadtime.Add(demand_distributions[i]);
				}
				demand_distributions_over_leadtime.push_back(DemOverLeadtime);
			}
			else {
				demand_distributions_over_leadtime.push_back(DiscreteDist::GetZeroDist());
			}

			totalDemandRate += demandRates[i];
			stockLevels.push_back(0);
		}
		class_config.Set("leadTimes", leadTimes);
		class_config.Set("holdingCosts", holdingCosts);
		class_config.Set("demandRates", demandRates);
		class_config.Set("highDemandVariance", highDemandVariance);
		class_config.Set("totalDemandRate", totalDemandRate);


		const int64_t max_iter = 5000;
		DetermineStockLevels(stockLevels, max_iter);
	} 

	std::vector<double> CalculateItemStatistics(int64_t item, int64_t stock_level) const
	{
		std::vector<double> statistics(4, 0.0);
		// expected fill rate, expected fill rate change, expected on hand inventory change, expected on hand inventory
		for (int64_t j = 0; j <= stock_level; j++) {
			const double leadTimeDemandProb = demand_distributions_over_leadtime[item].ProbabilityAt(j);
			double probSums = 0.0;
			for (int64_t k = 0; k <= stock_level - j; k++) {
				const double prob = demand_distributions[item].ProbabilityAt(k);
				probSums += prob;
				statistics[0] += leadTimeDemandProb * prob * k;
				statistics[3] += leadTimeDemandProb * prob * (stock_level - j - k);
			}
			const double factor = leadTimeDemandProb * (1.0 - probSums);
			statistics[0] += factor * (stock_level - j);
			statistics[1] += factor;
			statistics[2] += leadTimeDemandProb * probSums;
		}
		return statistics;
	}

	void DetermineStockLevels(std::vector<int64_t> stockLevels, int64_t max_iter)
	{
		double aggFillRate = 0.0;
		double expHoldingCost = 0.0;
		std::vector<double> ratio(numberOfItems, 0.0);
		std::vector<double> changeFR(numberOfItems, 0.0);
		std::vector<double> changeH(numberOfItems, 0.0);
		for (size_t i = 0; i < numberOfItems; i++)
		{
			const std::vector<double> statistics = CalculateItemStatistics(i, stockLevels[i]);
			aggFillRate += statistics[0];
			changeFR[i] = statistics[1];
			changeH[i] = statistics[2] * holdingCosts[i];
			ratio[i] = changeFR[i] / changeH[i];
		}
		aggFillRate /= totalDemandRate;

		double limit = -std::numeric_limits<double>::infinity();
		int64_t bestRatioSKU = 0;
		for (size_t i = 0; i < numberOfItems; i++)
		{
			if (ratio[i] > limit)
			{
				bestRatioSKU = i;
				limit = ratio[i];
			}
		}

		for (int64_t k = 0; k < max_iter; k++) {
			allBaseStockLevels.push_back(stockLevels);
			allExpectedPolicyFillRates.push_back(aggFillRate);
			allExpectedPolicyHoldingCosts.push_back(expHoldingCost);
			allExpectedPolicyStockoutAmounts.push_back(0.0);

			stockLevels[bestRatioSKU]++;
			const std::vector<double> statistics = CalculateItemStatistics(bestRatioSKU, stockLevels[bestRatioSKU]);
			double afr_change = statistics[1];
			double h_change = statistics[2] * holdingCosts[bestRatioSKU];
			ratio[bestRatioSKU] = afr_change / h_change;

			aggFillRate += changeFR[bestRatioSKU] / totalDemandRate;
			expHoldingCost += changeH[bestRatioSKU];
			changeFR[bestRatioSKU] = afr_change;
			changeH[bestRatioSKU] = h_change;
			double limit = -std::numeric_limits<double>::infinity();
			for (size_t i = 0; i < numberOfItems; i++)
			{
				if (ratio[i] > limit)
				{
					bestRatioSKU = i;
					limit = ratio[i];
				}
			}
		}
	}

	std::pair<double, double> TestPerformanceSinglePolicy(std::vector<int64_t> bs_levels) {
		auto& dp = DynaPlexProvider::Get();

		DynaPlex::VarGroup new_config = class_config;
		new_config.Set("concatBaseStockLevels", bs_levels);
		new_config.Set("totalActions", 1);
		new_config.Set("benchmarkAction", 0);
		DynaPlex::MDP mdp = dp.GetMDP(new_config);

		DynaPlex::VarGroup policy_config;
		policy_config.Add("id", "base_stock");
		policy_config.Set("serviceLevelPolicy", 0);
		auto policy = mdp->GetPolicy(policy_config);

		auto comparer = dp.GetPolicyComparer(mdp, test_config);
		auto comparison = comparer.Assess(policy);
		double cost;
		comparison.Get("mean", cost);
		double empricalExceededStockouts;
		comparison.Get("mean_stat_3", empricalExceededStockouts);
		simulationCount++;

		return { cost, empricalExceededStockouts / reviewHorizon };
	}

	int64_t return_j_max(double bestCost)
	{
		int64_t last_index = allExpectedPolicyFillRates.size() - 1;
		int64_t j_max = last_index;
		bool found = false;
		for (int64_t i = 1; i <= last_index; i++)
		{
			double holdingCost = allExpectedPolicyHoldingCosts[i];
			if (holdingCost > bestCost) {
				j_max = i - 1;
				found = true;
				break;
			}
		}
		if (!found) {
			std::vector<int64_t> bs_levels = allBaseStockLevels.back();
			double expHoldingCost = allExpectedPolicyHoldingCosts.back();
			std::vector<double> ratio(numberOfItems, 0.0);
			std::vector<double> changeFR(numberOfItems, 0.0);
			std::vector<double> changeH(numberOfItems, 0.0);
			for (size_t i = 0; i < numberOfItems; i++)
			{
				const std::vector<double> statistics = CalculateItemStatistics(i, bs_levels[i]);
				changeFR[i] = statistics[1];
				changeH[i] = statistics[2] * holdingCosts[i];
				ratio[i] = changeFR[i] / changeH[i];
			}

			double limit = -std::numeric_limits<double>::infinity();
			int64_t bestRatioSKU = 0;
			for (size_t i = 0; i < numberOfItems; i++)
			{
				if (ratio[i] > limit)
				{
					bestRatioSKU = i;
					limit = ratio[i];
				}
			}

			for (int64_t k = 0; k < 1; k++) {
				if (expHoldingCost < bestCost)
				{
					bs_levels[bestRatioSKU]++;
					expHoldingCost += changeH[bestRatioSKU];
					j_max++;
					const std::vector<double> statistics = CalculateItemStatistics(bestRatioSKU, bs_levels[bestRatioSKU]);
					changeFR[bestRatioSKU] = statistics[1];
					changeH[bestRatioSKU] = statistics[2] * holdingCosts[bestRatioSKU];
					ratio[bestRatioSKU] = changeFR[bestRatioSKU] / changeH[bestRatioSKU];
					double limit = -std::numeric_limits<double>::infinity();
					for (size_t i = 0; i < numberOfItems; i++)
					{
						if (ratio[i] > limit)
						{
							bestRatioSKU = i;
							limit = ratio[i];
						}
					}
				}
				else {
					j_max--;
					break;
				}
			}
		}
		return j_max;
	}

	int64_t return_j(double target)
	{
		int64_t last_index = allExpectedPolicyFillRates.size();
		int64_t j = 0;
		for (int64_t i = 0; i < last_index; i++)
		{
			double AFR = allExpectedPolicyFillRates[i];
			if (AFR >= target) {
				j = i;
				break;
			}
		}
		return j;
	}

	int64_t FindInfHorizonAction(int64_t j_obj) {
		double best_cost = std::numeric_limits<double>::infinity();
		int64_t bestPol = 0;
		for (int64_t i = 0; i <= j_obj; i++)
		{
			double infiniteHorizonStockout = totalDemandRate * std::max(0.0, (aggregateTargetFillRate - allExpectedPolicyFillRates[i]));
			double cost = allExpectedPolicyHoldingCosts[i] + infiniteHorizonStockout * penaltyCost;
			if (cost < best_cost) {
				bestPol = i;
				best_cost = cost;
			}
		}
		return bestPol;
	}

	void SplitPruneConquer(double targetAFR, int64_t reviewHorizonLength, double penalty, int64_t numberOfPossibleSlaTargets = 9, double incrementalFillRate = 0.005) {
		auto& dp = DynaPlexProvider::Get();

		aggregateTargetFillRate = targetAFR;
		class_config.Set("aggregateTargetFillRate", aggregateTargetFillRate);
		reviewHorizon = reviewHorizonLength;
		class_config.Set("reviewHorizon", reviewHorizon);
		penaltyCost = penalty;
		class_config.Set("penaltyCost", penaltyCost);
		simulationCount = 0;

		int64_t j_obj = return_j(aggregateTargetFillRate);
		double max_penalty = penaltyCost * aggregateTargetFillRate * totalDemandRate + penaltyCost / reviewHorizon;
		int64_t max_j_max = return_j_max(max_penalty);

		if (j_obj > max_j_max)
			dp.System() << "j_obj > max_j_max!!!" << std::endl;

		int64_t j_obj_inf = FindInfHorizonAction(j_obj);
		if (j_obj_inf < j_obj) 
			dp.System() << "j_obj_inf < j_obj!!!" << std::endl;

		std::vector<int64_t> bs_levels = allBaseStockLevels[j_obj];
		std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
		double new_per_period_shortfall = result.second;
		allExpectedPolicyStockoutAmounts[j_obj] = new_per_period_shortfall;
		double expHoldingCost = allExpectedPolicyHoldingCosts[j_obj];
		double cost_best = penaltyCost * new_per_period_shortfall + expHoldingCost;
		int64_t j_best = j_obj;
		unavoidableCostPerPeriod = expHoldingCost;
		double expectedFillRate = allExpectedPolicyFillRates[j_obj];
		const double expectedStockouts = (1.0 - expectedFillRate) * totalDemandRate;
		dp.System() << "Fr: " << expectedFillRate;
		dp.System() << "  St/o: " << expectedStockouts << "  shortfall: " << new_per_period_shortfall;
		dp.System() << "  holding costs:  " << expHoldingCost << "       ";
		dp.System() << "  total costs:  " << cost_best << "       ";
		for (int64_t stock : bs_levels) {
			dp.System() << "  " << stock;
		}
		dp.System() << std::endl;
		double objectiveCost = cost_best;
		double objectiveTarget = allExpectedPolicyFillRates[j_best];
		dp.System() << "Objective best static policy AFR: " << objectiveTarget << "  , objective best static cost:  " << objectiveCost << "  , unavoidable cost:  " << objectiveCost - unavoidableCostPerPeriod << std::endl;

		Split(j_best, cost_best, j_obj, numberOfPossibleSlaTargets, incrementalFillRate);
		incumbentTarget = allExpectedPolicyFillRates[j_best];
		double incumbentCost = cost_best;
		int64_t splitCount = simulationCount;
		dp.System() << "Incumbent best static policy AFR: " << incumbentTarget << "  , incumbent best static cost:  " << incumbentCost << "  , unavoidable cost:  " << incumbentCost - unavoidableCostPerPeriod << std::endl;
		Conquer(j_best, cost_best, j_obj);

		int64_t j_max = return_j_max(cost_best);
		int64_t last_policy_known = j_obj;
		if (setU.size() > 0)
			last_policy_known = setU.back();
		if (j_max > last_policy_known) {
			std::vector<int64_t> bs_levels = allBaseStockLevels[j_max];
			std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
			double new_per_period_shortfall = result.second;
			double expHoldingCost = allExpectedPolicyHoldingCosts[j_max];
			double cost_new = penaltyCost * new_per_period_shortfall + expHoldingCost;
			allExpectedPolicyStockoutAmounts[j_max] = new_per_period_shortfall;
			if (cost_new < cost_best) {
				j_best = j_max;
				cost_best = cost_new;
			}
			double candidate_cost = allExpectedPolicyHoldingCosts[last_policy_known] + penaltyCost * new_per_period_shortfall;
			if (candidate_cost < cost_best)
				Prune(j_best, cost_best, last_policy_known, j_max);
		}

		double max_best_penalty = penaltyCost * aggregateTargetFillRate * totalDemandRate;
		if (max_best_penalty < cost_best) {
			std::vector<int64_t> bs_levels = allBaseStockLevels[0];
			std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
			double new_per_period_shortfall = result.second;
			allExpectedPolicyStockoutAmounts[0] = new_per_period_shortfall;
			double cost_new = penaltyCost * new_per_period_shortfall;
			if (cost_new < cost_best) {
				j_best = 0;
				cost_best = cost_new;
			}
		}

		class_config.Set("unavoidableCostPerPeriod", unavoidableCostPerPeriod);
		greedyAction = j_obj;
		class_config.Set("greedyAction", greedyAction);
		benchmarkAction = j_best;
		best_static_cost = cost_best;

		best_static_afr = allExpectedPolicyFillRates[j_best];
		dp.System() << "Best static policy AFR: " << best_static_afr << "  , best static cost:  " << cost_best << "  , unavoidable cost:  " << cost_best - unavoidableCostPerPeriod << std::endl;
		dp.System() << "Gap to incumbent:  " << 100 * (cost_best - incumbentCost) / incumbentCost << "   gap to objective:  " << 100 * (cost_best - objectiveCost) / objectiveCost << std::endl;
		dp.System() << "Unavoidable Gap to incumbent:  " << 100 * (cost_best - incumbentCost) / (incumbentCost - unavoidableCostPerPeriod) << "   unavoidable gap to objective:  " << 100 * (cost_best - objectiveCost) / (objectiveCost - unavoidableCostPerPeriod) << std::endl;
		dp.System() << "Number of sim queries: " << simulationCount << "  , policies in split phase: " << splitCount << "  , j_max:  " << max_j_max << "  , best action:  " << benchmarkAction << std::endl;
	}

	void Prune(int64_t& j_best, double& cost_best, int64_t j_L, int64_t j_U) {
		int64_t index_diff = j_U - j_L;
		if (index_diff > 1) {
			int64_t j_M = (index_diff / 2) + j_L;
			std::vector<int64_t> bs_levels = allBaseStockLevels[j_M];
			std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
			double new_per_period_shortfall = result.second;
			double new_holding_cost = allExpectedPolicyHoldingCosts[j_M];
			double cost_new = penaltyCost * new_per_period_shortfall + new_holding_cost;
			allExpectedPolicyStockoutAmounts[j_M] = new_per_period_shortfall;

			if (cost_new < cost_best) {
				j_best = j_M;
				cost_best = cost_new;
			}

			double prev_h = allExpectedPolicyHoldingCosts[j_L];
			double candidate_cost_1 = prev_h + penaltyCost * new_per_period_shortfall;
			double candidate_cost_2 = new_holding_cost + penaltyCost * allExpectedPolicyStockoutAmounts[j_U];
			double min_candidate = std::min(candidate_cost_1, candidate_cost_2);

			if (min_candidate < cost_best) {
				if (candidate_cost_1 < candidate_cost_2) {
					Prune(j_best, cost_best, j_L, j_M);
					if (candidate_cost_2 < cost_best)
						Prune(j_best, cost_best, j_M, j_U);
				}
				else {
					Prune(j_best, cost_best, j_M, j_U);
					if (candidate_cost_2 < cost_best)
						Prune(j_best, cost_best, j_L, j_M);
				}
			}
		}
	}

	void Conquer(int64_t& j_best, double& cost_best, int64_t j_obj) {
		auto& dp = DynaPlexProvider::Get();

		int64_t j_L = j_obj;
		int64_t j_U = j_obj;
		int64_t setU_length = setU.size();
		int64_t setL_length = setL.size();

		for (int64_t i = 0; i < setU_length; i++) {
			int64_t new_index = setU[i];
			double current_shortfall = allExpectedPolicyStockoutAmounts[new_index];
			double candidate_cost = allExpectedPolicyHoldingCosts[j_L] + penaltyCost * current_shortfall;
			if (candidate_cost < cost_best)
				Prune(j_best, cost_best, j_L, new_index);
			j_L = new_index;
		}

		for (int64_t i = 0; i < setL_length; i++) {
			int64_t new_index = setL[i];
			double current_shortfall = allExpectedPolicyStockoutAmounts[j_U];
			double candidate_cost = allExpectedPolicyHoldingCosts[new_index] + penaltyCost * current_shortfall;
			if (candidate_cost < cost_best)
				Prune(j_best, cost_best, new_index, j_U);
			j_U = new_index;
		}
	}

	void Split(int64_t& j_best, double& cost_best, int64_t j_obj, int64_t numberOfPossibleSlaTargets, double incrementalFillRate) {

		auto& dp = DynaPlexProvider::Get();

		setL.clear();
		setU.clear();
		int64_t j_L = j_obj;
		int64_t j_U = j_obj;
		int64_t j_max = return_j_max(cost_best);

		for (int64_t k = 1; k <= numberOfPossibleSlaTargets; k++)
		{
			double AFR = aggregateTargetFillRate + incrementalFillRate * k;
			if (AFR < 1.0) {
				auto it = std::lower_bound(allExpectedPolicyFillRates.begin(), allExpectedPolicyFillRates.end(), AFR);
				int64_t index = std::distance(allExpectedPolicyFillRates.begin(), it);
				if (index > j_L) {
					if (index <= j_max) {
						std::vector<int64_t> bs_levels = allBaseStockLevels[index];
						std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
						double new_per_period_shortfall = result.second;
						double expHoldingCost = allExpectedPolicyHoldingCosts[index];
						double cost_new = penaltyCost * new_per_period_shortfall + expHoldingCost;
						if (cost_new < cost_best) {
							j_best = index;
							cost_best = cost_new;
							j_max = return_j_max(cost_best);
						}
						setU.push_back(index);
						j_L = index;
						allExpectedPolicyStockoutAmounts[index] = new_per_period_shortfall;

						double expectedFillRate = allExpectedPolicyFillRates[index];
						const double expectedStockouts = (1.0 - expectedFillRate) * totalDemandRate;
						dp.System() << "Fr: " << expectedFillRate;
						dp.System() << "  St/o: " << expectedStockouts << "  shortfall: " << new_per_period_shortfall;
						dp.System() << "  holding costs:  " << expHoldingCost << "       ";
						dp.System() << "  total costs:  " << cost_new << "       ";
						for (int64_t stock : bs_levels) {
							dp.System() << "  " << stock;
						}
						dp.System() << std::endl;
					}
				}
			}
		}
		for (int64_t k = 1; k <= numberOfPossibleSlaTargets; k++)
		{
			auto it = std::lower_bound(allExpectedPolicyFillRates.begin(), allExpectedPolicyFillRates.end(), aggregateTargetFillRate - incrementalFillRate * k);
			int64_t index = std::distance(allExpectedPolicyFillRates.begin(), it);
			double expHoldingCost = allExpectedPolicyHoldingCosts[index];
			double optimisticCost = penaltyCost * allExpectedPolicyStockoutAmounts[j_U] + expHoldingCost;
			if (index < j_U) {
				if (optimisticCost < cost_best) {
					std::vector<int64_t> bs_levels = allBaseStockLevels[index];
					std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
					double new_per_period_shortfall = result.second;
					double cost_new = penaltyCost * new_per_period_shortfall + expHoldingCost;
					if (cost_new < cost_best) {
						j_best = index;
						cost_best = cost_new;
						j_max = return_j_max(cost_best);
					}
					setL.push_back(index);
					j_U = index;
					allExpectedPolicyStockoutAmounts[index] = new_per_period_shortfall;

					double expectedFillRate = allExpectedPolicyFillRates[index];
					const double expectedStockouts = (1.0 - expectedFillRate) * totalDemandRate;
					dp.System() << "Fr: " << expectedFillRate;
					dp.System() << "  St/o: " << expectedStockouts << "  shortfall: " << new_per_period_shortfall;
					dp.System() << "  holding costs:  " << expHoldingCost << "       ";
					dp.System() << "  total costs:  " << cost_new << "       ";
					for (int64_t stock : bs_levels) {
						dp.System() << "  " << stock;
					}
					dp.System() << std::endl;
				}
			}
		}
		if (j_U > 0 && penaltyCost * allExpectedPolicyStockoutAmounts[j_U] < cost_best) 
			setL.push_back(0);
	}

	void FormCompositeActions(int64_t numberOfPossibleSlaTargets = 9, double incrementalFillRate = 0.005)
	{
		compositeActions.clear();
		for (int64_t k = numberOfPossibleSlaTargets; k >= 1; k--)
		{
			double AFR = best_static_afr - incrementalFillRate * k;
			auto it = std::lower_bound(allExpectedPolicyFillRates.begin(), allExpectedPolicyFillRates.end(), AFR);
			int64_t index = std::distance(allExpectedPolicyFillRates.begin(), it);
			compositeActions.push_back(index);
		}
		compositeActions.push_back(benchmarkAction);
		for (int64_t k = 1; k <= numberOfPossibleSlaTargets; k++)
		{
			double AFR = best_static_afr + incrementalFillRate * k;
			if (AFR < 1.0) {
				auto it = std::lower_bound(allExpectedPolicyFillRates.begin(), allExpectedPolicyFillRates.end(), AFR);
				int64_t index = std::distance(allExpectedPolicyFillRates.begin(), it);
				compositeActions.push_back(index);
			}
		}
	}

	void PrepareMDPforDCL(std::vector<int64_t> compositeActionsDCL)
	{
		totalActions = compositeActionsDCL.size();
		class_config.Set("totalActions", totalActions);
		std::vector<int64_t> concatBaseStockLevels;
		for (int64_t i = 0; i < totalActions; i++) {
			int64_t index = compositeActionsDCL[i];
			std::vector<int64_t> bs_levels = allBaseStockLevels[index];
			for (int64_t bs_level : bs_levels)
				concatBaseStockLevels.push_back(bs_level);
		}
		class_config.Set("concatBaseStockLevels", concatBaseStockLevels);
		auto it = std::find(compositeActionsDCL.begin(), compositeActionsDCL.end(), benchmarkAction);
		int64_t index = std::distance(compositeActionsDCL.begin(), it);
		benchmarkAction = index;
		class_config.Set("benchmarkAction", benchmarkAction);
	}

	void TestCosts(double targetAFR, int64_t reviewHorizonLength, double penalty)
	{
		auto& dp = DynaPlexProvider::Get();

		aggregateTargetFillRate = targetAFR;
		class_config.Set("aggregateTargetFillRate", aggregateTargetFillRate);
		reviewHorizon = reviewHorizonLength;
		class_config.Set("reviewHorizon", reviewHorizon);
		penaltyCost = penalty;
		class_config.Set("penaltyCost", penaltyCost);

		int64_t last_index = allExpectedPolicyFillRates.size() - 1;

		std::vector<double> holdingCost;
		std::vector<double> penaltyCost;
		std::vector<double> totalCost;
		std::vector<int64_t> nonUnimodalIndices;
		for (int64_t k = 0; k <= last_index; k++)
		{
			std::vector<int64_t> bs_levels = allBaseStockLevels[k];
			std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
			double expPenaltyCost = result.second * penalty;
			double expHoldingCost = allExpectedPolicyHoldingCosts[k];
			double exptotalCost = expPenaltyCost + expHoldingCost;

			holdingCost.push_back(expHoldingCost);
			penaltyCost.push_back(expPenaltyCost);
			totalCost.push_back(exptotalCost);

			if (k >= 2) {
				double last_cost = totalCost[k - 1];
				double last_cost_2 = totalCost[k - 2];
				if (last_cost > last_cost_2 && last_cost > exptotalCost) {
					nonUnimodalIndices.push_back(k - 1);
				}
			}
		}
		for (int64_t k = 0; k <= last_index; k++)
		{
			dp.System() << holdingCost[k] << ", ";
		}
		dp.System() << std::endl;
		for (int64_t k = 0; k <= last_index; k++)
		{
			dp.System() << penaltyCost[k] << ", ";
		}
		dp.System() << std::endl;
		for (int64_t k = 0; k <= last_index; k++)
		{
			dp.System() << totalCost[k] << ", ";
		}
		dp.System() << std::endl;
		int64_t numUnimodalIndices = nonUnimodalIndices.size();
		for (int64_t k = 0; k < numUnimodalIndices; k++)
		{
			dp.System() << nonUnimodalIndices[k] << ", ";
		}
		dp.System() << std::endl;
	}

	void TestObjective(int64_t reviewHorizonLength, double penalty) {
		
		auto& dp = DynaPlexProvider::Get();

		std::vector<double> targetAFRs = { 0.93, 0.94, 0.95, 0.96, 0.97 };

		reviewHorizon = reviewHorizonLength;
		class_config.Set("reviewHorizon", reviewHorizon);
		penaltyCost = penalty;
		class_config.Set("penaltyCost", penaltyCost);

		for (double target : targetAFRs) {
			class_config.Set("aggregateTargetFillRate", target);

			for (double policyTargets : targetAFRs) {
				int64_t j_obj = return_j(policyTargets);
				std::vector<int64_t> bs_levels = allBaseStockLevels[j_obj];
				std::pair<double, double> result = TestPerformanceSinglePolicy(bs_levels);
				double expShortfall = result.second;
				dp.System() << "Target:  " << target << " ,  policy:  " << policyTargets << " ,  amount:  " << expShortfall << std::endl;;
			}
		}
	}
};


static void TrainDCL(InitialMDPProcessing base_pre_mdp, DynaPlex::VarGroup test_config, DynaPlex::VarGroup dcl_config) {
	auto& dp = DynaPlexProvider::Get();

	base_pre_mdp.FormCompositeActions();
	std::vector<int64_t> compositeActions = base_pre_mdp.compositeActions;
	base_pre_mdp.PrepareMDPforDCL(compositeActions);
	int64_t benchmarkAction = base_pre_mdp.benchmarkAction;

	dp.System() << "---------------" << std::endl;
	dp.System() << std::endl;

	DynaPlex::MDP mdp = dp.GetMDP(base_pre_mdp.class_config);
	DynaPlex::VarGroup policy_config;
	policy_config.Add("id", "base_stock");
	policy_config.Add("serviceLevelPolicy", benchmarkAction);
	auto best_bs_policy = mdp->GetPolicy(policy_config);
	auto dcl = dp.GetDCL(mdp, best_bs_policy, dcl_config);
	dcl.TrainPolicy();

	if (dp.System().WorldRank() == 0) {
		auto dcl_policies = dcl.GetPolicies();
		policy_config.Set("id", "greedy_dynamic");
		auto dynamic_pol = mdp->GetPolicy(policy_config);
		dcl_policies.push_back(dynamic_pol);

		auto comparer = dp.GetPolicyComparer(mdp, test_config);
		auto comparison = comparer.Compare(dcl_policies, 0, true, false);
		for (auto results : comparison) {
			dp.System() << results.Dump() << std::endl;
		}

		dp.System() << std::endl;
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
}

static void PaperTests(bool train = false, bool unimodality_test = false) {
	auto& dp = DynaPlexProvider::Get();

	DynaPlex::VarGroup config;
	DynaPlex::VarGroup test_config;
	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{256,128,128,128}}
	};
	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"max_training_epochs", 100},
		{"train_based_on_probs", false}
	};
	DynaPlex::VarGroup dcl_config{
		{"L", 1000},
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"enable_sequential_halving", true}
	};

	// constant parameters
	config.Add("id", "multi_item_sla");
	config.Add("sendBackUnits", false);
	config.Add("cost_limit", 10.0);
	config.Add("backOrderCost", 0.0);
	test_config.Add("warmup_periods", 100);
	test_config.Add("rng_seed", 10061994);
	test_config.Add("number_of_statistics", 8);

	// variable parameters
	test_config.Add("number_of_trajectories", 1000);
	test_config.Add("periods_per_trajectory", 5000);

	int64_t mini_batch = 256;
	int64_t num_generations = 1;
	int64_t N = 100000;
	int64_t M = 1000;
	int64_t H_factor = 2;

	std::vector<int64_t> reviewHorizons = { 90, 70, 50, 10 };
	std::vector<double> penaltyCosts = { 300.0, 1200.0, 2400.0 };
	std::vector<double> AFRTargets = { 0.98, 0.90, 0.85 };

	std::vector<int64_t> possibleNumberOfItems = { 15, 20, 30, 50, 100, 1000 };
	std::vector<int64_t> leadTimes = { 2, 6 };
	std::vector<double> maxDemandRateLimit = { 1.0, 10.0 };
	std::vector<double> highVarianceRatios = { 0.0, 1.0 };

	int64_t base_reviewHorizon = 30;
	double base_penalty = 600.0;
	double base_target = 0.95;
	int64_t base_numberOfItems = 20;
	config.Set("numberOfItems", base_numberOfItems);
	int64_t base_leadTimes = 4;
	config.Set("leadTime", base_leadTimes);
	double base_demand_rate = 5.0;
	config.Set("demand_rate_limit", base_demand_rate);
	double base_variance_ratio = 0.5;
	config.Set("high_variance_ratio", base_variance_ratio);

	nn_training.Set("mini_batch_size", mini_batch);
	dcl_config.Set("nn_training", nn_training);
	dcl_config.Set("N", N);
	dcl_config.Set("M", M);
	dcl_config.Set("num_gens", num_generations);
	dcl_config.Set("H", H_factor * base_reviewHorizon);

	dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
	dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
	dp.System() << "-----" << std::endl;
	InitialMDPProcessing base_pre_mdp(config, test_config);
	base_pre_mdp.class_config.Set("aggregateTargetFillRate", base_target);
	base_pre_mdp.class_config.Set("reviewHorizon", base_reviewHorizon);
	base_pre_mdp.class_config.Set("penaltyCost", base_penalty);
	base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, base_penalty);
	if (train)
		TrainDCL(base_pre_mdp, test_config, dcl_config);
	if (unimodality_test)
		base_pre_mdp.TestCosts(base_target, base_reviewHorizon, base_penalty);
	dp.System() << "---------------" << std::endl;
	dp.System() << std::endl;

	for (double targets : AFRTargets) {
		dp.System() << "Instance:  " << targets << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
		dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(targets, base_reviewHorizon, base_penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(targets, base_reviewHorizon, base_penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dp.System() << std::endl;

	for (int64_t reviewHorizon : reviewHorizons) {
		dp.System() << "Instance:  " << base_target << "  " << reviewHorizon << "  " << base_penalty << "  ";
		dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		dcl_config.Set("H", H_factor* reviewHorizon);
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(base_target, reviewHorizon, base_penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(base_target, reviewHorizon, base_penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dcl_config.Set("H", H_factor* base_reviewHorizon);
	dp.System() << std::endl;

	for (double penalty : penaltyCosts) {
		dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << penalty << "  ";
		dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(base_target, base_reviewHorizon, penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dp.System() << std::endl;

	for (int64_t leadTime : leadTimes) {
		dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
		dp.System() << leadTime << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		config.Set("leadTime", leadTime);
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, base_penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(base_target, base_reviewHorizon, base_penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dp.System() << std::endl;
	config.Set("leadTime", base_leadTimes);

	for (double demand_rate_limit : maxDemandRateLimit) {
		dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
		dp.System() << base_leadTimes << "  " << demand_rate_limit << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		config.Set("demand_rate_limit", demand_rate_limit);
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, base_penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(base_target, base_reviewHorizon, base_penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dp.System() << std::endl;
	config.Set("demand_rate_limit", base_demand_rate);

	for (double high_variance_ratio : highVarianceRatios) {
		dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
		dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << high_variance_ratio << "  " << base_numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		config.Set("high_variance_ratio", high_variance_ratio);
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, base_penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(base_target, base_reviewHorizon, base_penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dp.System() << std::endl;
	config.Set("high_variance_ratio", base_variance_ratio);

	for (int64_t numberOfItems : possibleNumberOfItems) {
		dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
		dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << numberOfItems << std::endl;
		dp.System() << "-----" << std::endl;
		config.Set("numberOfItems", numberOfItems);
		InitialMDPProcessing base_pre_mdp(config, test_config);
		base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, base_penalty);
		if (train)
			TrainDCL(base_pre_mdp, test_config, dcl_config);
		if (unimodality_test)
			base_pre_mdp.TestCosts(base_target, base_reviewHorizon, base_penalty);
		dp.System() << "---------------" << std::endl;
		dp.System() << std::endl;
	}
	dp.System() << std::endl;
}

static void ShowPolicyBehavior() {
	auto& dp = DynaPlexProvider::Get();

	DynaPlex::VarGroup config;
	DynaPlex::VarGroup test_config;

	// constant parameters
	config.Add("id", "multi_item_sla");
	config.Add("sendBackUnits", false);
	config.Add("cost_limit", 10.0);
	config.Add("backOrderCost", 0.0);
	test_config.Add("warmup_periods", 100);
	test_config.Add("rng_seed", 10061994);
	test_config.Add("number_of_statistics", 8);
	test_config.Add("max_period_count", 5000);
	test_config.Add("number_of_trajectories", 1000);
	test_config.Add("periods_per_trajectory", 5000);

	int64_t base_reviewHorizon = 30;
	double base_penalty = 600.0;
	double base_target = 0.95;
	int64_t base_numberOfItems = 20;
	config.Set("numberOfItems", base_numberOfItems);
	int64_t base_leadTimes = 4;
	config.Set("leadTime", base_leadTimes);
	double base_demand_rate = 5.0;
	config.Set("demand_rate_limit", base_demand_rate);
	double base_variance_ratio = 0.5;
	config.Set("high_variance_ratio", base_variance_ratio);

	dp.System() << "Instance:  " << base_target << "  " << base_reviewHorizon << "  " << base_penalty << "  ";
	dp.System() << base_leadTimes << "  " << base_demand_rate << "  " << base_variance_ratio << "  " << base_numberOfItems << std::endl;
	dp.System() << "-----" << std::endl;
	InitialMDPProcessing base_pre_mdp(config, test_config);

	base_pre_mdp.class_config.Set("aggregateTargetFillRate", base_target);
	base_pre_mdp.class_config.Set("reviewHorizon", base_reviewHorizon);
	base_pre_mdp.class_config.Set("penaltyCost", base_penalty);

	//base_pre_mdp.SplitPruneConquer(base_target, base_reviewHorizon, base_penalty);
	dp.System() << "---------------" << std::endl;
	dp.System() << std::endl;
	base_pre_mdp.FormCompositeActions();
	std::vector<int64_t> compositeActions = base_pre_mdp.compositeActions;
	base_pre_mdp.PrepareMDPforDCL(compositeActions);
	int64_t benchmarkAction = base_pre_mdp.benchmarkAction;

	DynaPlex::MDP mdp = dp.GetMDP(base_pre_mdp.class_config);
	DynaPlex::VarGroup policy_config;
	policy_config.Add("id", "base_stock");
	policy_config.Add("serviceLevelPolicy", benchmarkAction);
	auto best_bs_policy = mdp->GetPolicy(policy_config);
	auto path = dp.System().filepath("Multi_item_SLA", "dcl_policy");
	auto dcl_policy = dp.LoadPolicy(mdp, path);
	auto demonstrator = dp.GetDemonstrator(test_config);

	auto trace_dcl = demonstrator.GetTrace(mdp, dcl_policy);
	for (auto& step : trace_dcl)
	{  
		std::cout << step.Dump() << std::endl;
	}
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	auto trace_bsp = demonstrator.GetTrace(mdp, best_bs_policy);
	for (auto& step : trace_bsp)
	{  
		std::cout << step.Dump() << std::endl;
	}
}

int main() {
	
	bool train = true;
	bool unimodality_test = false;
	PaperTests(train, unimodality_test);
	//ShowPolicyBehavior();

	return 0;
}