#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"
#include <cmath>

namespace DynaPlex::Models {
	namespace lost_sales_general
	{
		int64_t MDP::GetL(const State& state) const
		{
			return 100;
		}

		int64_t MDP::GetH(const State& state) const
		{
			return 21;
		}

		int64_t MDP::GetM(const State& state) const
		{
			return 500;
		}

		int64_t MDP::GetReinitiateCounter(const State& state) const
		{
			return 100;
		}

		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;		
			vars.Add("valid_actions", MaxOrderSize + 1);
			vars.Add("discount_factor", discount_factor);
			
			return vars;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("p", p);
			std::vector<int64_t> demand_cycles;
			std::vector<double> mean_demand;
			std::vector<double> stdDemand;
			std::vector<double> leadtime_probs;

			config.Get("demand_cycles", demand_cycles);
			cycle_length = demand_cycles.size();
			config.Get("mean_demand", mean_demand);
			config.Get("stdDemand", stdDemand);
			if (mean_demand.size() != demand_cycles.size() || stdDemand.size() != demand_cycles.size())
				throw DynaPlex::Error("MDP instance: Size of mean/std demand should be equal to demand cycle size.");

			std::vector<std::vector<double>> demand_probs;
			demand_probs.reserve(cycle_length);
			min_true_demand.reserve(cycle_length);
			cumulativePMFs.reserve(cycle_length);
			for (int64_t i = 0; i < cycle_length; i++) {
				DynaPlex::DiscreteDist demand_dist = DiscreteDist::GetAdanEenigeResingDist(mean_demand[i], stdDemand[i]);
				min_true_demand.push_back(demand_dist.Min());
				std::vector<double> probs;
				probs.reserve(demand_dist.DistinctValueCount());
				std::vector<double> cumul_probs;
				cumul_probs.reserve(demand_dist.DistinctValueCount());
				double sum = 0.0;
				for (const auto& [qty, prob] : demand_dist) {
					probs.push_back(prob);
					sum += prob;
					cumul_probs.push_back(sum);
				}
				demand_probs.push_back(probs);
				cumulativePMFs.push_back(cumul_probs);
			}

			order_crossover = false;
			stochasticLeadtimes = false;
			if (config.HasKey("stochastic_leadtime"))
				config.Get("stochastic_leadtime", stochasticLeadtimes);
			
			if (!stochasticLeadtimes) {
				config.Get("leadtime", max_leadtime);
				min_leadtime = max_leadtime;
				std::vector<double> det_leadtime_probs(max_leadtime + 1, 0.0);
				leadtime_probs = det_leadtime_probs;
				leadtime_probs[max_leadtime] = 1.0;
			}
			else if (config.HasKey("leadtime_distribution")) {
				config.Get("leadtime_distribution", leadtime_probs);

				if (config.HasKey("order_crossover"))
					config.Get("order_crossover", order_crossover);

				double total_prob = 0.0;
				bool found_min = false;
				for (int64_t i = 0; i < leadtime_probs.size(); i++) {
					double prob = leadtime_probs[i];
					if (prob < 0.0)
						throw DynaPlex::Error("MDP instance: lead time probability is negative.");
					total_prob += prob;
					if (!found_min && prob > 0.0) {
						min_leadtime = i;
						found_min = true;
					}
					if (std::abs(total_prob - 1.0) < 1e-8) {
						max_leadtime = i;
						break;
					}
				}
				if (std::abs(total_prob - 1.0) >= 1e-8)
					throw DynaPlex::Error("MDP instance: total lead time probability should be 1.0.");

				if (order_crossover) {
					std::vector<double> dummy_prob_vec(max_leadtime + 1, 1.0);
					cumulative_leadtime_probs = dummy_prob_vec;
					double total_prob = 1.0;
					for (int64_t i = 0; i < max_leadtime; i++) {
						cumulative_leadtime_probs[i] = leadtime_probs[i] / total_prob;
						total_prob -= leadtime_probs[i];
					}
				}
				else {
					std::vector<double> dummy_prob_vec(max_leadtime + 1, 0.0);
					cumulative_leadtime_probs = dummy_prob_vec;
					double total_prob = 0.0;
					for (int64_t i = 0; i <= max_leadtime; i++) {
						cumulative_leadtime_probs[i] = leadtime_probs[i] + total_prob;
						total_prob += leadtime_probs[i];
					}
					std::vector<double> probs(max_leadtime + 1, 0.0);
					leadtime_probs = probs;
					double total_probs = 0.0;
					for (int64_t i = 0; i <= max_leadtime; i++) {
						double prob = 1.0;
						for (int64_t j = 0; j < i; j++) {
							prob *= std::max(0.0, 1.0 - cumulative_leadtime_probs[j]);
						}
						prob *= std::max(0.0, cumulative_leadtime_probs[i]);
						leadtime_probs[i] = prob;
						total_probs += prob;
					}
					// Normalize the probabilities if the total differs from 1
					if (std::abs(total_probs - 1.0) >= 1e-8) {
						for (double& prob : leadtime_probs) {
							prob /= total_probs;
						}
					}
					total_probs = 0.0;
					for (const auto& prob : leadtime_probs)
					{
						if (prob < 0.0)
							throw DynaPlex::Error("MDP instance: non-crossover lead time probability is negative.");
						total_probs += prob;
					}
					if (std::abs(total_probs - 1.0) >= 1e-8)
						throw DynaPlex::Error("MDP instance: non-crossover total lead time probability should be 1.0.");
				}
			}
			else {
				throw DynaPlex::Error("MDP instance: Provide a leadtime value or leadtime distribution.");
			}

			h = 1.0;
			if (config.HasKey("maximizeRewards"))
				config.Get("maximizeRewards", maximizeRewards);
			else
				maximizeRewards = false;

			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;

			MaxOrderSize = 0;
			std::vector<double> probs_vec(leadtime_probs.begin() + min_leadtime, leadtime_probs.begin() + max_leadtime + 1);
			cycle_MaxOrderSize.reserve(cycle_length);
			cycle_MaxSystemInv.reserve(cycle_length);
			int64_t possible_leadtimes = max_leadtime - min_leadtime + 1;
			for (int64_t i = 0; i < cycle_length; i++) {
				std::vector<DiscreteDist> dist_vec;
				dist_vec.reserve(possible_leadtimes);
				std::vector<DiscreteDist> dist_vec_over_leadtime;
				dist_vec_over_leadtime.reserve(possible_leadtimes);
				for (int64_t j = min_leadtime; j <= max_leadtime; j++)
				{
					auto DemOverLeadtime = DiscreteDist::GetZeroDist();
					for (int64_t k = 0; k < j; k++) {
						int64_t cyclePeriod = (i + k) % cycle_length;
						DynaPlex::DiscreteDist dist_over_lt = DiscreteDist::GetCustomDist(demand_probs[cyclePeriod], min_true_demand[cyclePeriod]);
						DemOverLeadtime = DemOverLeadtime.Add(dist_over_lt);
					}
					int64_t cyclePeriod_on_leadtime = (i + j) % cycle_length;
					DynaPlex::DiscreteDist dist_on_leadtime = DiscreteDist::GetCustomDist(demand_probs[cyclePeriod_on_leadtime], min_true_demand[cyclePeriod_on_leadtime]);
					DemOverLeadtime = DemOverLeadtime.Add(dist_on_leadtime);
					dist_vec.push_back(dist_on_leadtime);
					dist_vec_over_leadtime.push_back(DemOverLeadtime);
				}
				auto DummyDemOverLeadtime = DiscreteDist::MultipleMix(dist_vec_over_leadtime, probs_vec);
				auto DummyDemOnLeadtime = DiscreteDist::MultipleMix(dist_vec, probs_vec);
				int64_t OrderSize = DummyDemOnLeadtime.Fractile(p / (p + h));
				MaxOrderSize = std::max(MaxOrderSize, OrderSize);
				cycle_MaxOrderSize.push_back(OrderSize);
				cycle_MaxSystemInv.push_back(DummyDemOverLeadtime.Fractile(p / (p + h)));
			}
		}

		double MDP::ModifyStateWithAction(State& state, int64_t action) const
		{	
			state.state_vector.push_back(action);
			state.total_inv += action;
			state.cat = StateCategory::AwaitEvent();
			return 0.0;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
				return action <= state.OrderConstraint;
		}

		MDP::Event MDP::GetEvent(const State& state, RNG& rng) const {
			double randomValue = rng.genUniform();
			// Use binary search on the cumulativePMF
			auto it = std::lower_bound(cumulativePMFs[state.period].begin(), cumulativePMFs[state.period].end(), randomValue);
			size_t index = std::distance(cumulativePMFs[state.period].begin(), it);
			int64_t demand = min_true_demand[state.period] + static_cast<int64_t>(index);

			if (stochasticLeadtimes) {
				std::vector<double> arrival_prob{};
				if (order_crossover) {
					int64_t last_order = state.state_vector.back();
					int64_t min_positive_leadtime = std::max((int64_t)1, min_leadtime);
					if (min_leadtime == 0) {
						for (int64_t i = 0; i < last_order; i++) {
							arrival_prob.push_back(rng.genUniform());
						}
						for (int64_t i = last_order; i < MaxOrderSize; i++) {
							rng.genUniform();
						}
					}
					for (int64_t j = min_positive_leadtime; j < max_leadtime; j++) {
						int64_t inv = state.state_vector.at(max_leadtime + 1 - j);
						for (int64_t i = 0; i < inv; i++) {
							arrival_prob.push_back(rng.genUniform());
						}
						for (int64_t i = inv; i < MaxOrderSize; i++) {
							rng.genUniform();
						}
					}
				}
				else {
					arrival_prob.push_back(rng.genUniform());
				}
				return { demand, arrival_prob };
			}
			else {
				return { demand, {} };
			}
		}

		double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const
		{
			state.cat = StateCategory::AwaitAction();
			int64_t onHand = state.state_vector.pop_front();
			int64_t new_coming_orders = 0;

			if (stochasticLeadtimes) {
				if (order_crossover) {
					int64_t action_num = 0;
					int64_t last_order = state.state_vector.back();
					int64_t min_positive_leadtime = std::max((int64_t)1, min_leadtime);
					if (min_leadtime == 0 && last_order > 0) {
						const double prob = cumulative_leadtime_probs[0];
						int64_t decrement_count = std::count_if(event.second.begin(), event.second.begin() + last_order,
							[prob](double value) { return value <= prob; });
						state.state_vector.back() -= decrement_count;
						onHand += decrement_count;
						action_num = last_order;
					}
					for (int64_t i = min_positive_leadtime; i < max_leadtime; i++) {
						int64_t& current_expected = state.state_vector.at(max_leadtime - i);
						if (current_expected > 0) {
							int64_t lb = action_num;
							int64_t ub = current_expected + lb;
							const double prob = cumulative_leadtime_probs[i];
							int64_t decrement_count = std::count_if(event.second.begin() + lb, event.second.begin() + ub,
								[prob](double value) { return value <= prob; });
							current_expected -= decrement_count;
							new_coming_orders += decrement_count;
							action_num = ub;
						}
					}
					int64_t& last_expected = state.state_vector.front();
					if (last_expected > 0) {
						new_coming_orders += last_expected;
						last_expected = 0;
					}					
				}
				else {
					double random_var = event.second.front();
					for (int64_t i = min_leadtime; i <= max_leadtime; i++) {
						if (random_var <= cumulative_leadtime_probs[i]) {
							int64_t base_loc = (i == 0 ? 1 : i);
							int64_t& earliest_received = state.state_vector.at(max_leadtime - base_loc);
							if (i == 0)
								onHand += earliest_received;
							else
								new_coming_orders += earliest_received;
							earliest_received = 0;
							for (int64_t j = i + 1; j <= max_leadtime; j++) {
								int64_t& received = state.state_vector.at(max_leadtime - j);
								new_coming_orders += received;
								received = 0;
							}
							break;
						}
					}
				}	
			}
			else { // deterministic leadtime
				int64_t expected = state.state_vector.front();
				if (expected > 0) {
					if (max_leadtime == 0)
						onHand += expected;
					else
						new_coming_orders += expected;
				}			
			}		

			int64_t demand = event.first;
			double cost =  0.0;
			double rewards = 0.0;
			state.cumulativeDemands += demand;

			if (onHand >= demand)
			{
				onHand -= demand;
				state.total_inv -= demand;
				cost = onHand * h;
				rewards = cost - demand * p;
			}
			else
			{
				int64_t stockouts = demand - onHand;
				state.total_inv -= onHand;
				cost = stockouts * p;
				rewards = -onHand * p;
				state.cumulativeStockouts += stockouts;

				onHand = 0;
			}
			state.state_vector.front() = onHand + new_coming_orders;

			if (state.cumulativeDemands > 0)
				state.ServiceLevel = static_cast<double>(state.cumulativeDemands - state.cumulativeStockouts) / (static_cast<double>(state.cumulativeDemands));

			state.period++;
			state.period = state.period % cycle_length;
			int64_t MaximumOrderSize = cycle_MaxOrderSize[state.period];
			int64_t MaxSystemInv = cycle_MaxSystemInv[state.period];
			state.OrderConstraint = std::max(static_cast<int64_t>(0), std::min(MaxSystemInv - state.total_inv, MaximumOrderSize));
			
			if (!maximizeRewards)
				return cost;
			else
				return rewards;
		}

		std::vector<double> MDP::ReturnUsefulStatistics(const State& state) const
		{
			return { state.ServiceLevel };
		}

		void MDP::ResetHiddenStateVariables(State& state, RNG& rng) const
		{
			state.ServiceLevel = 1.0;
			state.cumulativeDemands = 0;
			state.cumulativeStockouts = 0;
		}

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features) const {
			features.Add(state.state_vector);
			if (cycle_length > 1) 
				features.Add(state.period);
		}

		MDP::State MDP::GetInitialState() const
		{
			State state{};

			state.period = 0;
			state.ServiceLevel = 1.0;
			state.cumulativeDemands = 0;
			state.cumulativeStockouts = 0;

			auto queue = Queue<int64_t>{}; //queue for state vector
			int64_t length = std::max((int64_t)1, max_leadtime);
			queue.reserve(length + 1);
			queue.push_back(0);
			for (int64_t i = 1; i < max_leadtime; i++)
			{
				queue.push_back(0);
			}
			state.cat = StateCategory::AwaitAction();
			state.state_vector = queue;
			state.total_inv = queue.sum();

			int64_t MaximumOrderSize = cycle_MaxOrderSize[state.period];
			int64_t MaxSystemInv = cycle_MaxSystemInv[state.period];
			state.OrderConstraint = std::max(static_cast<int64_t>(0), std::min(MaxSystemInv - state.total_inv, MaximumOrderSize));
			return state;
		}


		MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("period", state.period);
			vars.Get("state_vector", state.state_vector);
			vars.Get("total_inv", state.total_inv);
			return state;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("period", period);
			vars.Add("state_vector", state_vector);
			vars.Add("total_inv", total_inv);
			return vars;
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}

		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
				/*=id though which the MDP will be retrievable*/ "lost_sales_general",
				/*description*/ "Lost sales problem with cyclic demand and stochastic lead times.)",
				/*reference to passed registry*/registry); 
		}

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<GreedyCappedBaseStockPolicy>("greedy_capped_base_stock",
				"Capped base-stock policy with suboptimal S and r.");
		}
	}
}

