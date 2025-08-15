#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"
#include <cmath>

namespace DynaPlex::Models {
	namespace Zero_Shot_Lost_Sales_Inventory_Control 
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

			VarGroup diagnostics{};			
			diagnostics.Add("MaxOrderSize", MaxOrderSize);
			diagnostics.Add("MaxSystemInv", MaxSystemInv);
			vars.Add("diagnostics", diagnostics);
			
			return vars;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("evaluate", evaluate);
			config.Get("train_stochastic_leadtimes", train_stochastic_leadtimes);
			config.Get("train_cyclic_demand", train_cyclic_demand);
			config.Get("train_random_yield", train_random_yield);
			config.Get("max_leadtime", max_leadtime);
			config.Get("max_demand", max_demand);
			config.Get("max_p", max_p);
			config.Get("max_num_cycles", max_num_cycles);
			h = 1.0;
			min_p = 2.0;
			min_leadtime = 0;
			min_demand = 2.0;
			min_randomYield = 0.75;
			maximizeRewards = false;
			
			if (evaluate) {				
				if (config.HasKey("censoredDemand"))
					config.Get("censoredDemand", censoredDemand);
				else
					censoredDemand = false;

				if (config.HasKey("maximizeRewards"))
					config.Get("maximizeRewards", maximizeRewards);
				else
					maximizeRewards = false;

				config.Get("p", p);
				order_crossover = false;
				bool stochastic_leadtime = false;
				if (config.HasKey("stochastic_leadtime"))
					config.Get("stochastic_leadtime", stochastic_leadtime);

				config.Get("demand_cycles", demand_cycles);
				config.Get("mean_demand", mean_demand);
				config.Get("stdDemand", stdDemand);
				if (mean_demand.size() != demand_cycles.size() || stdDemand.size() != demand_cycles.size())
					throw DynaPlex::Error("MDP instance: Size of mean/std demand should be equal to demand cycle size.");

				if (!stochastic_leadtime) {
					std::vector<double> probs(max_leadtime + 1, 0.0);
					leadtime_probs = probs;
					censoredLeadtime = false;
					int64_t leadtime;
					config.Get("leadtime", leadtime);
					if (leadtime > max_leadtime || leadtime < min_leadtime)
						throw DynaPlex::Error("MDP instance: Leadtime should be between max_leadtime and min_leadtime.");
					else
						leadtime_probs[leadtime] = 1.0;
				}
				else if (config.HasKey("leadtime_distribution")) {
					stochastic_leadtime = true;
					if (config.HasKey("censoredLeadtime"))
						config.Get("censoredLeadtime", censoredLeadtime);
					else
						censoredLeadtime = false;
					if (config.HasKey("order_crossover"))
						config.Get("order_crossover", order_crossover);

					if (order_crossover) {
						config.Get("leadtime_distribution", leadtime_probs);
						if (leadtime_probs.size() != max_leadtime + 1)
							throw DynaPlex::Error("MDP instance: Size of leadtime probability vector should be max_leadtime + 1.");
					}
					else {
						config.Get("leadtime_distribution", non_crossing_leadtime_rv_probs);
						if (non_crossing_leadtime_rv_probs.size() != max_leadtime + 1)
							throw DynaPlex::Error("MDP instance: Size of non_crossing_leadtime_rv_probs vector should be max_leadtime + 1.");
						double total_prob_v2 = 0.0;
						for (const auto& prob : non_crossing_leadtime_rv_probs)
						{
							if (prob < 0.0)
								throw DynaPlex::Error("MDP instance: non-crossover lead time probability is negative.");
							total_prob_v2 += prob;
						}
						if (std::abs(total_prob_v2 - 1.0) >= 1e-8)
							throw DynaPlex::Error("MDP instance: non-crossover total lead time probability should be 1.0.");

						std::vector<double> cumul_probs(max_leadtime + 1, 0.0);
						double total_prob_v1 = 0.0;
						for (int64_t i = 0; i <= max_leadtime; i++) {
							cumul_probs[i] = non_crossing_leadtime_rv_probs[i] + total_prob_v1;
							total_prob_v1 += non_crossing_leadtime_rv_probs[i];
						}
						std::vector<double> probs(max_leadtime + 1, 0.0);
						leadtime_probs = probs;
						double total_probs = 0.0;
						for (int64_t i = 0; i <= max_leadtime; i++) {
							double prob = 1.0;
							for (int64_t j = 0; j < i; j++) {
								prob *= std::max(0.0, 1.0 - cumul_probs[j]);
							}
							prob *= std::max(0.0, cumul_probs[i]);
							leadtime_probs[i] = prob;
							total_probs += prob;
						}
						// Normalize the probabilities if the total differs from 1
						if (std::abs(total_probs - 1.0) >= 1e-8) {
							for (double& prob : leadtime_probs) {
								prob /= total_probs;
							}
						}
					}
					double total_probs = 0.0;
					for (const auto& prob : leadtime_probs)
					{
						if (prob < 0.0)
							throw DynaPlex::Error("MDP instance: lead time probability is negative.");
						total_probs += prob;
					}
					if (std::abs(total_probs - 1.0) >= 1e-8)
						throw DynaPlex::Error("MDP instance: total lead time probability should be 1.0.");
				}
				else {
					throw DynaPlex::Error("MDP instance: Provide a leadtime value or leadtime distribution.");
				}

				if (config.HasKey("randomYield")) {
					config.Get("randomYield", randomYield);
				}
				else {
					randomYield = false;
					censoredRandomYield = false;
				}
				
				if (randomYield) {
					if (config.HasKey("censoredRandomYield"))
						config.Get("censoredRandomYield", censoredRandomYield);
					else
						censoredRandomYield = false;
					config.Get("yield_when_realized", yield_when_realized);
					config.Get("randomYield_case", randomYield_case);
					if (randomYield_case == 0) {
						if (stochastic_leadtime && order_crossover) {
							config.Get("random_yield_probs_crossover", random_yield_probs_crossover);
						}
						else {
							throw DynaPlex::Error("MDP instance: random_yield_probs for randomYield_case == 0 not supported yet.");
						}
					}
					else if (randomYield_case == 1) {
						config.Get("min_yield", min_yield);
						if (min_yield < min_randomYield)
							throw DynaPlex::Error("MDP instance: min_yield should be greater than min_randomYield used when training.");
						if (min_yield > 1.0)
							throw DynaPlex::Error("MDP instance: min_yield should be less than 1.0.");
					}
					else {
						config.Get("random_yield_dist", random_yield_dist);
						if (randomYield_case == 3) {
							config.Get("p_var", p_var);
							if (p_var > 1.0)
								throw DynaPlex::Error("MDP instance: p_var should be <= 1.0.");
							config.Get("alpha_var", alpha_var);
							if (alpha_var <= 0.0)
								throw DynaPlex::Error("MDP instance: alpha_var should be > 0.0.");
						}
						else if (randomYield_case == 4) {
							config.Get("k_var", k_var);
							if (k_var <= 0.0)
								throw DynaPlex::Error("MDP instance: k_var should be > 0.0.");
						}
						else if (randomYield_case != 2) {
							throw DynaPlex::Error("MDP instance: Provide a feasible random yield model: 0 - 1 - 2 - 3 - 4.");
						}
					}
				}
			}

			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;

			DynaPlex::DiscreteDist dist = DiscreteDist::GetAdanEenigeResingDist(max_demand, max_demand * 2);
			//Initiate members that are computed from the parameters:
			auto DemOverLeadtime = DiscreteDist::GetZeroDist();
			for (size_t i = 0; i <= max_leadtime; i++)
			{
				DemOverLeadtime = DemOverLeadtime.Add(dist);
			}
			MaxOrderSize = dist.Fractile(max_p / (max_p + h));
			MaxSystemInv = DemOverLeadtime.Fractile(max_p / (max_p + h));
			if (train_random_yield) 
				MaxOrderSize = static_cast<int64_t>(std::floor((double) MaxOrderSize * (1 / min_randomYield)));
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
			auto it = std::lower_bound(state.cumulativePMFs[state.period].begin(), state.cumulativePMFs[state.period].end(), randomValue);
			size_t index = std::distance(state.cumulativePMFs[state.period].begin(), it);
			int64_t demand = state.min_true_demand[state.period] + static_cast<int64_t>(index);

			if (state.stochasticLeadtimes) {
				std::vector<double> arrival_prob{};
				if (state.order_crossover) {
					int64_t last_order = state.state_vector.back();
					int64_t min_positive_leadtime = std::max((int64_t)1, state.min_leadtime);
					if (state.min_leadtime == 0) {
						for (int64_t i = 0; i < last_order; i++) {
							arrival_prob.push_back(rng.genUniform());
						}
						for (int64_t i = last_order; i < state.MaxOrderSize_Limit; i++) {
							rng.genUniform();
						}
					}
					for (int64_t j = min_positive_leadtime; j < state.max_leadtime; j++) {
						int64_t inv = state.state_vector.at(max_leadtime + 1 - j);
						for (int64_t i = 0; i < inv; i++) {
							arrival_prob.push_back(rng.genUniform());
						}
						for (int64_t i = inv; i < state.MaxOrderSize_Limit; i++) {
							rng.genUniform();
						}
					}
					if (state.randomYield) {
						if (state.yield_when_realized) {
							for (int64_t j = state.max_leadtime; j >= min_positive_leadtime; j--) {
								int64_t inv = state.state_vector.at(max_leadtime + 1 - j);
								for (int64_t i = 0; i < inv; i++) {
									arrival_prob.push_back(rng.genUniform());
								}
								for (int64_t i = inv; i < state.MaxOrderSize_Limit; i++) {
									rng.genUniform();
								}
							}
							if (state.min_leadtime == 0) {
								for (int64_t i = 0; i < last_order; i++) {
									arrival_prob.push_back(rng.genUniform());
								}
								for (int64_t i = last_order; i < state.MaxOrderSize_Limit; i++) {
									rng.genUniform();
								}
							}
						}
						else {
							arrival_prob.push_back(rng.genUniform());
						}
					}
				}
				else {
					arrival_prob.push_back(rng.genUniform());
					if (state.randomYield) {
						if (state.yield_when_realized) {
							for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
								arrival_prob.push_back(rng.genUniform());
							}
						}
						else {
							arrival_prob.push_back(rng.genUniform());
						}
					}
				}
				return { demand, arrival_prob };
			}
			else if (state.randomYield) {
				return { demand, { rng.genUniform() } };
			}
			else {
				return { demand, {} };
			}
		}

		double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const
		{
			state.cat = StateCategory::AwaitAction();
			int64_t orders_received = state.orders_received;
			int64_t onHand = state.state_vector.pop_front();
			int64_t new_coming_orders = 0;

			if (state.stochasticLeadtimes) {
				if (!evaluate) { // training happpens here -- w or w/out order crossover and w or w/out random yield
					if (state.order_crossover) { // training - order crossover
						int64_t action_num = 0;
						int64_t last_order = state.state_vector.back();
						int64_t min_positive_leadtime = std::max((int64_t)1, state.min_leadtime);
						if (!state.randomYield) { // training - order crossover no yield
							if (state.min_leadtime == 0 && last_order > 0) {
								const double prob = state.cumulative_leadtime_probs[0];
								int64_t decrement_count = std::count_if(event.second.begin(), event.second.begin() + last_order,
									[prob](double value) { return value <= prob; });
								state.state_vector.back() -= decrement_count;
								onHand += decrement_count;
								action_num = last_order;
							}
							for (int64_t i = min_positive_leadtime; i < state.max_leadtime; i++) {
								int64_t& current_expected = state.state_vector.at(max_leadtime - i);
								if (current_expected > 0) {
									int64_t lb = action_num;
									int64_t ub = current_expected + lb;
									const double prob = state.cumulative_leadtime_probs[i];
									int64_t decrement_count = std::count_if(event.second.begin() + lb, event.second.begin() + ub,
										[prob](double value) { return value <= prob; });
									current_expected -= decrement_count;
									new_coming_orders += decrement_count;
									action_num = ub;
								}
							}
							int64_t& last_expected = state.state_vector.at(max_leadtime - state.max_leadtime);
							if (last_expected > 0) {
								new_coming_orders += last_expected;
								last_expected = 0;
							}
						}
						else { // training - order crossover random yield
							state.dummy_pipeline_vector.push_back(last_order);
							if (state.yield_when_realized) { // training - order crossover random yield realized when received
								const int64_t event_size = static_cast<int64_t>(event.second.size()) - 1;
								if (state.min_leadtime == 0 && last_order > 0) {
									const double prob_leadtime = state.cumulative_leadtime_probs[0];
									const double prob_yield = state.random_yield_probs_crossover[last_order];
									int64_t decrement_count = 0;
									for (action_num = 0; action_num < last_order; action_num++) {
										if (event.second[action_num] <= prob_leadtime) {
											decrement_count++;
											if (event.second[event_size - action_num] <= prob_yield)
												onHand++;
											else
												state.total_inv--;
										}
									}
									state.state_vector.back() -= decrement_count;
								}
								for (int64_t i = min_positive_leadtime; i < state.max_leadtime; i++) {
									int64_t index = max_leadtime - i;
									int64_t& expected = state.state_vector.at(index);
									if (expected > 0) {
										int64_t lb = action_num;
										int64_t ub = expected + lb;
										const double prob_leadtime = state.cumulative_leadtime_probs[i];
										const int64_t order_placed = state.dummy_pipeline_vector.at(index);
										const double prob_yield = state.random_yield_probs_crossover[order_placed];
										int64_t decrement_count = 0;
										for (action_num = lb; action_num < ub; action_num++) {
											if (event.second[action_num] <= prob_leadtime) {
												decrement_count++;
												if (event.second[event_size - action_num] <= prob_yield)
													new_coming_orders++;
												else
													state.total_inv--;
											}
										}
										expected -= decrement_count;
									}
								}
								int64_t& last_expected = state.state_vector.at(max_leadtime - state.max_leadtime);
								if (last_expected > 0) {
									const int64_t order_placed = state.dummy_pipeline_vector.at(max_leadtime - state.max_leadtime);
									const double prob_yield = state.random_yield_probs_crossover[order_placed];
									for (int64_t i = 0; i < last_expected; i++) {
										if (event.second[event_size - action_num - i] <= prob_yield)
											new_coming_orders++;
										else
											state.total_inv--;
									}
									last_expected = 0;
								}
							}
							else { // training - order crossover random yield realized before shipment
								int64_t to_be_received = (last_order > 0) ? OrderArrivals(state, last_order, event.second.back(), false) : 0;
								state.pipeline_vector.push_back(to_be_received);
								if (state.min_leadtime == 0 && last_order > 0) {
									if (to_be_received > 0) {
										const double prob = state.cumulative_leadtime_probs[0];
										int64_t decrement_count = std::count_if(event.second.begin(), event.second.begin() + to_be_received,
											[prob](double value) { return value <= prob; });
										state.state_vector.back() -= decrement_count;
										state.pipeline_vector.back() -= decrement_count;
										onHand += decrement_count;
									}
									action_num = last_order;
								}
								for (int64_t i = min_positive_leadtime; i < state.max_leadtime; i++) {
									int64_t& current_expected = state.state_vector.at(max_leadtime - i);
									int64_t& current_realized = state.pipeline_vector.at(max_leadtime - i);
									int64_t lb = action_num;
									if (current_realized > 0) {
										int64_t ub = current_realized + lb;
										const double prob = state.cumulative_leadtime_probs[i];
										int64_t decrement_count = std::count_if(event.second.begin() + lb, event.second.begin() + ub,
											[prob](double value) { return value <= prob; });
										current_expected -= decrement_count;
										current_realized -= decrement_count;
										new_coming_orders += decrement_count;
									}
									action_num = current_expected + lb;
								}
								int64_t& last_expected = state.state_vector.at(max_leadtime - state.max_leadtime);
								if (last_expected > 0) {
									int64_t& last_realized = state.pipeline_vector.at(max_leadtime - state.max_leadtime);
									state.total_inv -= (last_expected - last_realized);
									new_coming_orders += last_realized;
									last_expected = 0;
									last_realized = 0;
								}
								state.pipeline_vector.pop_front();
							}
							state.dummy_pipeline_vector.pop_front();
						}
					}
					else { // training - no crossover
						double random_var = event.second.front();
						if (!state.randomYield) { // training - no crossover no yield
							for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
								if (random_var <= state.cumulative_leadtime_probs[i]) {
									int64_t base_loc = (i == 0 ? 1 : i);
									int64_t& earliest_received = state.state_vector.at(max_leadtime - base_loc);
									if (i == 0)
										onHand += earliest_received;
									else
										new_coming_orders += earliest_received;
									earliest_received = 0;
									for (int64_t j = i + 1; j <= state.max_leadtime; j++) {
										int64_t& received = state.state_vector.at(max_leadtime - j);
										new_coming_orders += received;
										received = 0;
									}
									break;
								}
							}
						}
						else { // training - no crossover random yield
							if (state.yield_when_realized) { // training - no crossover random yield realized when received
								for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
									if (random_var <= state.cumulative_leadtime_probs[i]) {
										int64_t base_loc = (i == 0 ? 1 : i);
										int64_t& earliest_expected = state.state_vector.at(max_leadtime - base_loc);
										if (earliest_expected > 0) {
											int64_t earliest_received = OrderArrivals(state, earliest_expected, event.second[i - state.min_leadtime + 1], false);
											if (i == 0)
												onHand += earliest_received;
											else
												new_coming_orders += earliest_received;
											state.total_inv -= (earliest_expected - earliest_received);
											earliest_expected = 0;
										}
										for (int64_t j = i + 1; j <= state.max_leadtime; j++) {
											int64_t& expected = state.state_vector.at(max_leadtime - j);
											if (expected > 0) {
												int64_t received = OrderArrivals(state, expected, event.second[j - state.min_leadtime + 1], false);
												new_coming_orders += received;
												state.total_inv -= (expected - received);
												expected = 0;
											}
										}
										break;
									}
								}
							}
							else { // training - no crossover random yield realized before shipment
								int64_t last_order = state.state_vector.back();
								int64_t to_be_received = (last_order > 0) ? OrderArrivals(state, last_order, event.second.back(), false) : 0;
								state.pipeline_vector.push_back(to_be_received);
								for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
									int64_t base_loc = (i == 0 ? 1 : i);
									int64_t& earliest_received = state.pipeline_vector.at(max_leadtime - base_loc);
									if ((random_var <= state.cumulative_leadtime_probs[i] && earliest_received > 0) || i == state.max_leadtime) {
										int64_t& earliest_expected = state.state_vector.at(max_leadtime - base_loc);
										if (i == 0)
											onHand += earliest_received;
										else
											new_coming_orders += earliest_received;
										state.total_inv -= (earliest_expected - earliest_received);
										earliest_expected = 0;
										earliest_received = 0;
										for (int64_t j = i + 1; j <= state.max_leadtime; j++) {
											int64_t& received = state.pipeline_vector.at(max_leadtime - j);
											int64_t& expected = state.state_vector.at(max_leadtime - j);
											state.total_inv -= (expected - received);
											new_coming_orders += received;
											expected = 0;
											received = 0;
										}
										break;
									}
								}
								state.pipeline_vector.pop_front();
							}
						}
					}
				}
				else { // evaluate - inference time
					if (state.order_crossover) { // evaluate order crossover
						int64_t action_num = 0;
						int64_t last_order = state.state_vector.back();
						int64_t min_positive_leadtime = std::max((int64_t)1, state.min_leadtime);
						if (!state.randomYield) { // evaluate order crossover no yield
							if (state.min_leadtime == 0 && last_order > 0) {
								const double prob = state.cumulative_leadtime_probs[0];
								int64_t decrement_count = std::count_if(event.second.begin(), event.second.begin() + last_order,
									[prob](double value) { return value <= prob; });
								state.state_vector.back() -= decrement_count;
								onHand += decrement_count;
								action_num = last_order;
								if (state.censoredLeadtime) {
									state.past_leadtimes[0] += decrement_count;
									state.orders_received += decrement_count;
								}
							}
							for (int64_t i = min_positive_leadtime; i < state.max_leadtime; i++) {
								int64_t& current_expected = state.state_vector.at(max_leadtime - i);
								if (current_expected > 0) {
									int64_t lb = action_num;
									int64_t ub = current_expected + lb;
									const double prob = state.cumulative_leadtime_probs[i];
									int64_t decrement_count = std::count_if(event.second.begin() + lb, event.second.begin() + ub,
										[prob](double value) { return value <= prob; });
									current_expected -= decrement_count;
									new_coming_orders += decrement_count;
									action_num = ub;
									if (state.censoredLeadtime) {
										state.past_leadtimes[i] += decrement_count;
										state.orders_received += decrement_count;
									}
								}
							}
							int64_t& last_expected = state.state_vector.at(max_leadtime - state.max_leadtime);
							if (last_expected > 0) {
								new_coming_orders += last_expected;
								if (state.censoredLeadtime) {
									state.past_leadtimes[state.max_leadtime] += last_expected;
									state.orders_received += last_expected;
								}
								last_expected = 0;
							}
						}
						else { // evaluate order crossover random yield
							state.dummy_pipeline_vector.push_back(last_order);
							if (state.yield_when_realized) { // evaluate order crossover random yield realized when received
								const int64_t event_size = static_cast<int64_t>(event.second.size()) - 1;
								if (state.min_leadtime == 0 && last_order > 0) {
									int64_t initial_onHand = onHand;
									const double prob_leadtime = state.cumulative_leadtime_probs[0];
									const double prob_yield = state.random_yield_probs_crossover[last_order];
									int64_t decrement_count = 0;
									for (action_num = 0; action_num < last_order; action_num++) {
										if (event.second[action_num] <= prob_leadtime) {
											decrement_count++;
											if (event.second[event_size - action_num] <= prob_yield)
												onHand++;
											else
												state.total_inv--;
										}
									}
									state.state_vector.back() -= decrement_count;
									if (state.censoredLeadtime) {
										state.past_leadtimes[0] += decrement_count;
										state.orders_received += decrement_count;
									}
									if (state.censoredRandomYield) {
										state.random_yield_statistics[last_order].first += onHand - initial_onHand;
										state.random_yield_statistics[last_order].second += decrement_count;										
									}
								}
								for (int64_t i = min_positive_leadtime; i < state.max_leadtime; i++) {
									int64_t index = max_leadtime - i;
									int64_t& expected = state.state_vector.at(index);
									if (expected > 0) {
										int64_t lb = action_num;
										int64_t ub = expected + lb;
										const double prob_leadtime = state.cumulative_leadtime_probs[i];
										const int64_t order_placed = state.dummy_pipeline_vector.at(index);
										const double prob_yield = state.random_yield_probs_crossover[order_placed];
										int64_t decrement_count = 0;
										for (action_num = lb; action_num < ub; action_num++) {
											if (event.second[action_num] <= prob_leadtime) {
												decrement_count++;
												if (event.second[event_size - action_num] <= prob_yield)
													new_coming_orders++;
												else
													state.total_inv--;
											}
										}
										expected -= decrement_count;
										if (state.censoredLeadtime) {
											state.past_leadtimes[i] += decrement_count;
											state.orders_received += decrement_count;
										}
										if (state.censoredRandomYield) {
											state.random_yield_statistics[order_placed].first += new_coming_orders;
											state.random_yield_statistics[order_placed].second += decrement_count;
										}
									}
								}
								int64_t& last_expected = state.state_vector.at(max_leadtime - state.max_leadtime);
								if (last_expected > 0) {
									const int64_t order_placed = state.dummy_pipeline_vector.at(max_leadtime - state.max_leadtime);
									const double prob_yield = state.random_yield_probs_crossover[order_placed];
									int64_t received_until = new_coming_orders;
									for (int64_t i = 0; i < last_expected; i++) {
										if (event.second[event_size - action_num - i] <= prob_yield)
											new_coming_orders++;
										else
											state.total_inv--;
									}
									if (state.censoredLeadtime) {
										state.past_leadtimes[state.max_leadtime] += last_expected;
										state.orders_received += last_expected;
									}
									if (state.censoredRandomYield) {
										state.random_yield_statistics[order_placed].first += new_coming_orders - received_until;
										state.random_yield_statistics[order_placed].second += last_expected;
									}
									last_expected = 0;
								}
							}
							else { // evaluate order crossover random yield realized before shipment
								int64_t to_be_received = (last_order > 0) ? OrderArrivals(state, last_order, event.second.back(), false) : 0;
								state.pipeline_vector.push_back(to_be_received);
								state.received_orders_vector.push_back(to_be_received);
								if (state.min_leadtime == 0 && last_order > 0) {
									if (to_be_received > 0) {
										const double prob = state.cumulative_leadtime_probs[0];
										int64_t decrement_count = std::count_if(event.second.begin(), event.second.begin() + to_be_received,
											[prob](double value) { return value <= prob; });
										state.state_vector.back() -= decrement_count;
										state.pipeline_vector.back() -= decrement_count;
										onHand += decrement_count;
										if (state.censoredLeadtime) {
											state.past_leadtimes[0] += decrement_count;
											state.orders_received += decrement_count;
										}
									}
									action_num = last_order;
									if (state.censoredRandomYield && state.state_vector.back() == 0) {
										state.random_yield_statistics[last_order].first += last_order;
										state.random_yield_statistics[last_order].second += last_order;
										state.dummy_pipeline_vector.back() = 0;
									}
								}
								for (int64_t i = min_positive_leadtime; i < state.max_leadtime; i++) {
									int64_t& current_expected = state.state_vector.at(max_leadtime - i);
									int64_t& current_realized = state.pipeline_vector.at(max_leadtime - i);
									int64_t lb = action_num;
									action_num = current_expected + lb;
									if (current_realized > 0) {
										int64_t ub = current_realized + lb;
										const double prob = state.cumulative_leadtime_probs[i];
										int64_t decrement_count = std::count_if(event.second.begin() + lb, event.second.begin() + ub,
											[prob](double value) { return value <= prob; });
										current_expected -= decrement_count;
										current_realized -= decrement_count;
										new_coming_orders += decrement_count;
										if (state.censoredLeadtime) {
											state.past_leadtimes[i] += decrement_count;
											state.orders_received += decrement_count;
										}
									}
									if (state.censoredRandomYield && current_expected == 0) {
										int64_t& received = state.dummy_pipeline_vector.at(max_leadtime - i);
										state.random_yield_statistics[received].first += received;
										state.random_yield_statistics[received].second += received;
										received = 0;
									}
								}
								int64_t& last_expected = state.state_vector.at(max_leadtime - state.max_leadtime);
								if (last_expected > 0) {
									int64_t& last_realized = state.pipeline_vector.at(max_leadtime - state.max_leadtime);
									new_coming_orders += last_realized;
									last_expected -= last_realized;
									if (state.censoredRandomYield && last_expected == 0) {
										int64_t& received = state.dummy_pipeline_vector.at(max_leadtime - state.max_leadtime);
										state.random_yield_statistics[received].first += received;
										state.random_yield_statistics[received].second += received;
										received = 0;
									}
									if (state.censoredLeadtime) {
										state.past_leadtimes[state.max_leadtime] += last_realized;
										state.orders_received += last_realized;
									}
									else {
										last_expected = 0;
									}
								}
								int64_t lead_time_threshold = state.censoredLeadtime ? max_leadtime : state.max_leadtime;
								int64_t expected_orders = state.dummy_pipeline_vector.at(max_leadtime - lead_time_threshold);
								int64_t received_orders = state.received_orders_vector.at(max_leadtime - lead_time_threshold);
								if (expected_orders > 0) {
									state.total_inv -= (expected_orders - received_orders);
									if (state.censoredRandomYield) {
										state.random_yield_statistics[expected_orders].first += received_orders;
										state.random_yield_statistics[expected_orders].second += expected_orders;
									}
								}
								state.pipeline_vector.pop_front();
								state.received_orders_vector.pop_front();
							}
							state.dummy_pipeline_vector.pop_front();
						}
					}
					else { // evaluate no crossover 
						double random_var = event.second.front();
						if (!state.randomYield) { // evaluate no crossover no yield  
							for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
								int64_t base_loc = (i == 0 ? 1 : i);
								int64_t& earliest_received = state.state_vector.at(max_leadtime - base_loc);
								if (random_var <= state.cumulative_leadtime_probs[i] && earliest_received > 0) {
									int64_t last_observed = i;
									if (i == 0)
										onHand += earliest_received;
									else
										new_coming_orders += earliest_received;
									earliest_received = 0;
									for (int64_t j = i + 1; j <= state.max_leadtime; j++) {
										int64_t& received = state.state_vector.at(max_leadtime - j);
										if (received > 0) {
											last_observed = j;
											new_coming_orders += received;
											received = 0;
										}
									}
									if (state.censoredLeadtime) {
										for (int64_t j = i; j <= last_observed; j++) {
											state.past_leadtimes[j]++;
											state.orders_received++;
										}
									}
									break;
								}
							}
						}
						else { // evaluate no crossover random yield 
							if (state.yield_when_realized) { // evaluate no crossover random yield realized when received
								for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
									int64_t base_loc = (i == 0 ? 1 : i);
									int64_t& earliest_expected = state.state_vector.at(max_leadtime - base_loc);
									if (random_var <= state.cumulative_leadtime_probs[i] && earliest_expected > 0) {
										int64_t last_observed = i;
										int64_t earliest_received = OrderArrivals(state, earliest_expected, event.second[i - state.min_leadtime + 1]);
										if (i == 0)
											onHand += earliest_received;
										else
											new_coming_orders += earliest_received;
										state.total_inv -= (earliest_expected - earliest_received);
										earliest_expected = 0;
										for (int64_t j = i + 1; j <= state.max_leadtime; j++) {
											int64_t& expected = state.state_vector.at(max_leadtime - j);
											if (expected > 0) {
												last_observed = j;											
												int64_t received = OrderArrivals(state, expected, event.second[j - state.min_leadtime + 1]);
												new_coming_orders += received;
												state.total_inv -= (expected - received);
												expected = 0;
											}
										}
										if (state.censoredLeadtime) {
											for (int64_t j = i; j <= last_observed; j++) {
												state.past_leadtimes[j]++;
												state.orders_received++;
											}
										}
										break;
									}
								}
							}
							else { // evaluate no crossover random yield realized before shipment
								int64_t last_order = state.state_vector.back();
								int64_t to_be_received = (last_order > 0) ? OrderArrivals(state, last_order, event.second.back(), false) : 0;
								state.pipeline_vector.push_back(to_be_received);
								for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
									int64_t base_loc = (i == 0 ? 1 : i);
									int64_t& earliest_received = state.pipeline_vector.at(max_leadtime - base_loc);
									if (random_var <= state.cumulative_leadtime_probs[i] && earliest_received > 0) {
										int64_t last_observed = i;
										int64_t& earliest_expected = state.state_vector.at(max_leadtime - base_loc);
										if (state.censoredRandomYield) {
											state.random_yield_statistics[earliest_expected].first += earliest_received;
											state.random_yield_statistics[earliest_expected].second += earliest_expected;
										}
										state.total_inv -= (earliest_expected - earliest_received);
										if (i == 0)
											onHand += earliest_received;
										else
											new_coming_orders += earliest_received;
										earliest_received = 0;
										earliest_expected = 0;
										for (int64_t j = i + 1; j <= state.max_leadtime; j++) {
											int64_t& received = state.pipeline_vector.at(max_leadtime - j);
											int64_t& expected = state.state_vector.at(max_leadtime - j);
											if (expected > 0) {
												last_observed = j;
												state.total_inv -= (expected - received);
												new_coming_orders += received;
												if (state.censoredRandomYield) {
													state.random_yield_statistics[expected].first += received;
													state.random_yield_statistics[expected].second += expected;
												}
												received = 0;
												expected = 0;
											}
										}
										if (state.censoredLeadtime) {
											for (int64_t j = i; j <= last_observed; j++) {
												state.past_leadtimes[j]++;
												state.orders_received++;
											}
										}
										else {
											for (int64_t j = state.max_leadtime + 1; j <= max_leadtime; j++) {
												int64_t& expected = state.state_vector.at(max_leadtime - j);
												if (expected > 0) {
													state.total_inv -= expected;
													if (state.censoredRandomYield) {
														state.random_yield_statistics[expected].first += 0;
														state.random_yield_statistics[expected].second += expected;
													}
													expected = 0;
												}
											}
										}
										break;
									}
								}
								int64_t& expected = state.censoredLeadtime ? state.state_vector.front() : state.state_vector.at(max_leadtime - state.max_leadtime);
								if (expected > 0) {
									state.total_inv -= expected;
									if (state.censoredRandomYield) {
										state.random_yield_statistics[expected].first += 0;
										state.random_yield_statistics[expected].second += expected;
									}
									expected = 0;
								}
								state.pipeline_vector.pop_front();
							}
						}
					}
				}
			}
			else { // deterministic leadtime
				int64_t loc = (state.max_leadtime == 0 ? 1 : state.max_leadtime);
				int64_t& expected = state.state_vector.at(max_leadtime - loc);
				if (!state.randomYield) { // deterministic leadtime no yield
					if (expected > 0) {
						if (state.max_leadtime == 0)
							onHand += expected;
						else
							new_coming_orders += expected;
						expected = 0;
					}
				}
				else { // deterministic leadtime random yield
					if (state.yield_when_realized) { // deterministic leadtime random yield realized when received
						if (expected > 0) {
							int64_t received = OrderArrivals(state, expected, event.second.front(), state.censoredRandomYield);
							state.total_inv -= (expected - received);
							if (state.max_leadtime == 0)
								onHand += received;
							else
								new_coming_orders += received;
							expected = 0;
						}
					}
					else { // deterministic leadtime random yield realized before shipment
						int64_t last_order = state.state_vector.back();
						int64_t to_be_received = (last_order > 0) ? OrderArrivals(state, last_order, event.second.front(), false) : 0;
						state.pipeline_vector.push_back(to_be_received);
						if (expected > 0) {
							int64_t& received = state.pipeline_vector.at(max_leadtime - loc);
							state.total_inv -= (expected - received);
							if (state.censoredRandomYield) {
								state.random_yield_statistics[expected].first += received;
								state.random_yield_statistics[expected].second += expected;
							}
							if (state.max_leadtime == 0)
								onHand += received;
							else
								new_coming_orders += received;
							expected = 0;
							received = 0;
						}
						state.pipeline_vector.pop_front();
					}
				}
			}

			if (state.censoredDemand) {
				if (!state.collectDemandStatistics[state.period] && onHand > 0)
					state.collectDemandStatistics[state.period] = true;
			}				

			int64_t demand = event.first;
			state.demand = demand;

			double cost =  0.0;
			double rewards = 0.0;
			bool uncensored = true;
			if (evaluate) 
				state.cumulativeDemands += demand;

			if (onHand >= demand)
			{
				onHand -= demand;
				state.total_inv -= demand;
				cost = onHand * h;
				rewards = cost - demand * state.p;
			}
			else
			{
				int64_t stockouts = demand - onHand;
				state.total_inv -= onHand;
				cost = stockouts * state.p;
				rewards = -onHand * state.p;

				if (evaluate) 
					state.cumulativeStockouts += stockouts;

				if (state.censoredDemand) {
					uncensored = false;
					demand = onHand;
				} 

				onHand = 0;
			}
			state.state_vector.front() = onHand + new_coming_orders;

			if (evaluate && state.cumulativeDemands > 0)
				state.ServiceLevel = static_cast<double>(state.cumulativeDemands - state.cumulativeStockouts) / (static_cast<double>(state.cumulativeDemands));

			if (state.collectStatistics) {
				if (state.censoredLeadtime && state.orders_received > orders_received)
					UpdateLeadTimeStatistics(state);
				if (state.censoredDemand && state.collectDemandStatistics[state.period])
					UpdateDemandStatistics(state, uncensored, demand); // Call Kaplan - Meier Estimator		
				int64_t old_period = state.period;
				state.period++;
				state.period = state.period % state.cycle_length;
				UpdateOrderLimits(state);
			}
			else {
				state.period++;
				state.period = state.period % state.cycle_length;
				state.MaxOrderSize = state.cycle_MaxOrderSize[state.period];
				state.MaxSystemInv = state.cycle_MaxSystemInv[state.period];
			}

			if (state.randomYield) {
				int64_t MaxOrder = state.MaxOrderSize;
				if (state.censoredRandomYield) {
					state.random_yield_features = UpdateRandomYieldFeatures(state);
					bool OrderSizeFound = false;
					for (int64_t i = state.MaxOrderSize; i <= state.MaxOrderSize_Limit; i++) {
						if (static_cast<int64_t>(std::floor(i * state.random_yield_features[i])) >= state.MaxOrderSize) {
							MaxOrder = i;
							OrderSizeFound = true;
							break;
						}
					}
					if (!OrderSizeFound)
						MaxOrder = state.MaxOrderSize_Limit;
				}

				double expected_total_inv = static_cast<double>(state.state_vector.front());
				for (int64_t i = 1; i < max_leadtime; i++) {
					int64_t expected_order = state.state_vector.at(i);
					if (expected_order > 0) {
						if (state.order_crossover) {
							expected_total_inv += expected_order * state.random_yield_features[state.dummy_pipeline_vector.at(i - 1)];
						}
						else {
							expected_total_inv += expected_order * state.random_yield_features[expected_order];
						}
					}
				}
				int64_t effective_total_inv = static_cast<int64_t>(std::floor(expected_total_inv));
				state.OrderConstraint = std::max(static_cast<int64_t>(0), std::min(state.MaxSystemInv - effective_total_inv, MaxOrder));
			}
			else {
				state.OrderConstraint = std::max(static_cast<int64_t>(0), std::min(state.MaxSystemInv - state.total_inv, state.MaxOrderSize));
			}
			
			if (train_random_yield)
				state.random_yield_nn_features = GetRandomYieldFeatures(state);

			if (!maximizeRewards)
				return cost;
			else
				return rewards;
		}

		int64_t MDP::OrderArrivals(State& state, int64_t num_orders_expected, double random_val, bool updateStatistics) const{
			int64_t received_orders = num_orders_expected;
			if (!evaluate || randomYield_case == 0) {
				std::vector<double> probs = state.random_yield_probs[num_orders_expected];
				for (int64_t i = num_orders_expected; i >= 0; i--) {
					if (probs[i] >= random_val) {
						received_orders = i;
						break;
					}
				}
			}
			else {
				if (randomYield_case == 1) {
					double random_part = min_yield + (1.0 - min_yield) * random_val;
					received_orders = static_cast<int64_t>(std::round(random_part * num_orders_expected));
				}
				else {
					int64_t random_yield_variable = random_yield_dist.GetSampleFromProb(random_val);
					if (randomYield_case == 2) {
						received_orders = std::min(num_orders_expected, random_yield_variable);
					}
					else if (randomYield_case == 3) {
						double pow_result = std::pow(static_cast<double>(random_yield_variable), static_cast<double>(p_var));
						int64_t order_received = static_cast<int64_t>(std::ceil(num_orders_expected * random_yield_variable / (num_orders_expected + alpha_var * pow_result)));
						received_orders = std::min(num_orders_expected, order_received);
					}
					else if (randomYield_case == 4) {
						int64_t order_received = static_cast<int64_t>(std::ceil(num_orders_expected * k_var / (num_orders_expected + random_yield_variable)));
						received_orders = std::min(num_orders_expected, order_received);
					}
				}
			}

			if (state.censoredRandomYield && updateStatistics) {
				state.random_yield_statistics[num_orders_expected].first += received_orders;
				state.random_yield_statistics[num_orders_expected].second += num_orders_expected;
			}

			return received_orders;
		}

		void MDP::UpdateOrderLimits(State& state) const {
			std::vector<double> probs_vec(state.estimated_leadtime_probs.begin() + state.estimated_min_leadtime, state.estimated_leadtime_probs.begin() + state.estimated_max_leadtime + 1);
			int64_t possible_leadtimes = state.estimated_max_leadtime - state.estimated_min_leadtime + 1;
			std::vector<DiscreteDist> dist_vec;
			dist_vec.reserve(possible_leadtimes);
			std::vector<DiscreteDist> dist_vec_over_leadtime;
			dist_vec_over_leadtime.reserve(possible_leadtimes);
			for (int64_t j = state.estimated_min_leadtime; j <= state.estimated_max_leadtime; j++)
			{
				auto DemOverLeadtime = DiscreteDist::GetZeroDist();
				for (int64_t k = 0; k < j; k++) {
					int64_t cyclePeriod = (state.period + k) % state.cycle_length;
					DynaPlex::DiscreteDist dist_over_lt = DiscreteDist::GetCustomDist(state.cycle_probs[cyclePeriod], state.cycle_min_demand[cyclePeriod]);
					DemOverLeadtime = DemOverLeadtime.Add(dist_over_lt);
				}
				int64_t cyclePeriod_on_leadtime = (state.period + j) % state.cycle_length;
				DynaPlex::DiscreteDist dist_on_leadtime = DiscreteDist::GetCustomDist(state.cycle_probs[cyclePeriod_on_leadtime], state.cycle_min_demand[cyclePeriod_on_leadtime]);
				DemOverLeadtime = DemOverLeadtime.Add(dist_on_leadtime);
				dist_vec.push_back(dist_on_leadtime);
				dist_vec_over_leadtime.push_back(DemOverLeadtime);
			}
			auto DummyDemOnLeadtime = DiscreteDist::MultipleMix(dist_vec, probs_vec);
			auto DummyDemOverLeadtime = DiscreteDist::MultipleMix(dist_vec_over_leadtime, probs_vec);
			state.MaxOrderSize = std::min(DummyDemOnLeadtime.Fractile(state.p / (state.p + h)), state.MaxOrderSize_Limit);
			state.MaxSystemInv = std::min(DummyDemOverLeadtime.Fractile(state.p / (state.p + h)), MaxSystemInv);
		}

		void MDP::UpdateLeadTimeStatistics(State& state) const {
			std::vector<int64_t> dummy_past_leadtimes = state.past_leadtimes;
			int64_t dummy_orders_received = state.orders_received;
			if (state.estimated_max_leadtime < max_leadtime) {
				int64_t to_be_received = 0;
				for (int64_t i = std::max((int64_t) 1, state.estimated_max_leadtime); i < max_leadtime; i++) {
					to_be_received += state.state_vector.at(max_leadtime - i);
				}
				if (to_be_received > 0) {
					int64_t leadtimes_inbetween = max_leadtime - state.estimated_max_leadtime;
					int64_t fractional_received = static_cast<int64_t>(std::floor((double) to_be_received / leadtimes_inbetween));
					int64_t total_distributed = 0;
					for (int64_t i = state.estimated_max_leadtime + 1; i < max_leadtime; i++) {
						dummy_past_leadtimes[i] += fractional_received;
						total_distributed += fractional_received;
					}
					dummy_past_leadtimes[max_leadtime] += (to_be_received - total_distributed);
					dummy_orders_received += to_be_received;
				}
			}

			for (int64_t i = 0; i <= max_leadtime; i++) {
				state.estimated_leadtime_probs[i] = static_cast<double>(dummy_past_leadtimes[i]) / static_cast<double>(dummy_orders_received);
			}

			bool found_min = false;
			double total_prob = 0.0;
			for (int64_t i = 0; i <= max_leadtime; i++) {
				double prob = state.estimated_leadtime_probs[i];
				total_prob += prob;
				if (!found_min && prob > 0.0) {
					state.estimated_min_leadtime = i;
					found_min = true;
				}
				if (std::abs(total_prob - 1.0) < 1e-8) {
					state.estimated_max_leadtime = i;
					break;
				}
			}
		} 

		void MDP::UpdateDemandStatistics(State& state, bool uncensored, int64_t newObs) const { // Kaplan - Meier Estimator
			int64_t current_cyclePeriod = state.demand_cycles[state.period];
			state.periodCount[current_cyclePeriod]++;

			int64_t oldSize = state.past_demands[current_cyclePeriod].size() - 1;
			if (newObs > oldSize) {
				for (int64_t i = oldSize + 1; i < newObs; i++) {
					state.past_demands[current_cyclePeriod].push_back(0);
					state.censor_indicator[current_cyclePeriod].push_back(0);
					state.cumulative_demands[current_cyclePeriod].push_back(1);
				}
				state.past_demands[current_cyclePeriod].push_back(1);
				state.cumulative_demands[current_cyclePeriod].push_back(0);

				if (uncensored) {
					state.censor_indicator[current_cyclePeriod].push_back(0);
				}
				else {
					state.censor_indicator[current_cyclePeriod].push_back(1);
				}
			}
			else {
				state.past_demands[current_cyclePeriod][newObs]++;
				oldSize = newObs - 1;

				if (!uncensored) {
					state.censor_indicator[current_cyclePeriod][newObs]++;
				}
			}
			for (int64_t i = 0; i < oldSize + 1; i++) {
				state.cumulative_demands[current_cyclePeriod][i]++;
			}

			int64_t demand_size = state.past_demands[current_cyclePeriod].size();
			std::vector<double> probs(demand_size, 0.0);
			// Iterative weight redistribution
			for (int64_t i = 0; i < demand_size; i++) {
				probs[i] += static_cast<double>(state.past_demands[current_cyclePeriod][i]) / state.periodCount[current_cyclePeriod];
				if (state.censor_indicator[current_cyclePeriod][i] > 0 && i < demand_size - 1) { // Censored observation
					double weight_to_redistribute = static_cast<double>(state.censor_indicator[current_cyclePeriod][i]) / state.periodCount[current_cyclePeriod];
					probs[i] -= weight_to_redistribute;

					for (int64_t j = i + 1; j < demand_size; ++j) {
						probs[j] += state.past_demands[current_cyclePeriod][j] * weight_to_redistribute / state.cumulative_demands[current_cyclePeriod][i];
					}
				}
			}
			for (int64_t i = 0; i < state.cycle_length; i++) {
				if (state.demand_cycles[i] == current_cyclePeriod) {
					state.cycle_probs[i] = probs;
					state.cycle_min_demand[i] = 0;
					DynaPlex::DiscreteDist dist = DiscreteDist::GetCustomDist(probs, 0);
					state.mean_cycle_demand[i] = dist.Expectation();
					state.std_cycle_demand[i] = dist.StandardDeviation();
					state.periodCount[i] = state.periodCount[current_cyclePeriod];
					state.past_demands[i] = state.past_demands[current_cyclePeriod];
					state.censor_indicator[i] = state.censor_indicator[current_cyclePeriod];
					state.cumulative_demands[i] = state.cumulative_demands[current_cyclePeriod];
				}
			}
		}

		std::vector<double> MDP::UpdateRandomYieldFeatures(const State& state) const {
			std::vector<double> random_yield_means(MaxOrderSize + 1, 1.0);
			bool max_order_found = false;
			double max_order_ratio = 0.0;
			for (int64_t i = 1; i <= MaxOrderSize; i++) {
				if (state.random_yield_statistics[i].second > 0) {
					random_yield_means[i] = static_cast<double>(state.random_yield_statistics[i].first) / state.random_yield_statistics[i].second;
					max_order_found = true;
					max_order_ratio = random_yield_means[i];
				}
				else if (max_order_found) {
					random_yield_means[i] = max_order_ratio;
				}
			}

			for (size_t i = 1; i <= MaxOrderSize; ++i) {
				if (random_yield_means[i] > random_yield_means[i - 1]) { // If the sequence is increasing				
					double sum = random_yield_means[i] + random_yield_means[i - 1];
					int64_t count = 2;
					int64_t j = i - 1;
					// Pool adjacent violators
					while (j > 0 && random_yield_means[j - 1] < sum / count) {
						sum += random_yield_means[j - 1];
						count++;
						j--;
					}
					// Adjust the pooled values
					double adjustedValue = sum / count;
					for (int64_t k = j; k <= i; ++k) {
						random_yield_means[k] = adjustedValue;
					}
				}
			}

			return random_yield_means;
		}

		std::vector<double> MDP::GetRandomYieldFeatures(const State& state) const {
			if (include_all_features) {
				std::vector<double> random_yield_features(MaxOrderSize + 1, 1.0);
				random_yield_features[0] = 0.0;
				for (int64_t i = 1; i <= state.OrderConstraint; i++) {
					random_yield_features[i] = state.random_yield_features[i];
				}
				for (int64_t i = state.OrderConstraint + 1; i <= MaxOrderSize; i++) {
					random_yield_features[i] = 0.0;
				}		
				return random_yield_features;
			}
			else {
				std::vector<double> random_yield_features(randomYield_features_size, 1.0);
				double size = static_cast<double>(state.OrderConstraint) / randomYield_features_size;
				int64_t currentIndex = 0;
				for (int64_t i = 0; i < randomYield_features_size; ++i) {
					if (i < state.OrderConstraint % randomYield_features_size) 
						currentIndex += std::ceil(size);		
					else 
						currentIndex += std::floor(size);				
					random_yield_features[i] = state.random_yield_features[currentIndex];
				}
				return random_yield_features;
			}
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
			if (train_stochastic_leadtimes) {
				if (state.order_crossover)
					features.Add(1);
				else
					features.Add(0);
			}
			features.Add(state.p);
			features.Add(state.state_vector);
			if (train_stochastic_leadtimes) {
				for (int64_t i = max_leadtime; i >= 0; i--) {
					features.Add(state.estimated_leadtime_probs[i]);
				}
			}
			else {
				features.Add(state.min_leadtime);
			}
			if (train_cyclic_demand) {
				features.Add(state.cycle_length);
				for (int64_t i = 0; i < max_num_cycles; i++) {
					int64_t cyclePeriod = (state.period + i) % state.cycle_length;
					features.Add(state.mean_cycle_demand[cyclePeriod]);
					features.Add(state.std_cycle_demand[cyclePeriod]);
				}
			}
			else {
				features.Add(state.mean_cycle_demand.front());
				features.Add(state.std_cycle_demand.front());
			}
			if (train_random_yield) {
				if(!include_all_features)
					features.Add(state.OrderConstraint);
				features.Add(state.random_yield_nn_features);
			}
		}

		MDP::State MDP::GetInitialState(RNG& rng) const
		{
			State state{};

			state.period = 0;
			state.demand = 0;
			state.collectStatistics = false;
			state.censoredDemand = false;
			state.censoredLeadtime = false;
			state.censoredRandomYield = false;
			std::vector<double> mean_true_demand;
			std::vector<double> stdev_true_demand;
			std::vector<double> leadtime_true_probs;
			DynaPlex::DiscreteDist edge_dist = DiscreteDist::GetConstantDist(static_cast<int64_t>(std::ceil(max_demand)));

			if (evaluate) {
				state.ServiceLevel = 1.0;
				state.cumulativeDemands = 0;
				state.cumulativeStockouts = 0;
				state.demand_cycles = demand_cycles;
				state.cycle_length = demand_cycles.size();
				mean_true_demand = mean_demand;
				stdev_true_demand = stdDemand;
				state.mean_cycle_demand = mean_demand;
				state.std_cycle_demand = stdDemand;
				leadtime_true_probs = leadtime_probs;
				bool found_min = false;
				double total_prob = 0.0;
				for (int64_t i = 0; i <= max_leadtime; i++) {
					double prob = leadtime_true_probs[i];
					total_prob += prob;
					if (!found_min && prob > 0.0) {
						state.min_leadtime = i;
						found_min = true;
					}
					if (std::abs(total_prob - 1.0) < 1e-8) {
						state.max_leadtime = i;
						break;
					}
				}
				state.estimated_leadtime_probs = leadtime_true_probs;
				state.estimated_max_leadtime = state.max_leadtime;
				state.estimated_min_leadtime = state.min_leadtime;

				state.p = p;
				state.randomYield = randomYield;
				if (state.randomYield) {
					state.yield_when_realized = yield_when_realized;
					state.censoredRandomYield = censoredRandomYield;
				}

				if (censoredDemand) {
					state.collectDemandStatistics.reserve(state.cycle_length);
					state.periodCount.reserve(state.cycle_length);
					state.past_demands.reserve(state.cycle_length);
					state.cumulative_demands.reserve(state.cycle_length);
					state.censor_indicator.reserve(state.cycle_length);
					for (int64_t i = 0; i < state.cycle_length; i++) {
						state.collectDemandStatistics.push_back(false);
						state.mean_cycle_demand[i] = edge_dist.Expectation();
						state.std_cycle_demand[i] = edge_dist.StandardDeviation();
						state.periodCount.push_back(0);
						state.past_demands.push_back({});
						state.cumulative_demands.push_back({});
						state.censor_indicator.push_back({});
					}
					state.censoredDemand = true;
					state.collectStatistics = true;
				}

				if (censoredLeadtime) {
					state.past_leadtimes.reserve(max_leadtime + 1);
					state.estimated_min_leadtime = max_leadtime;
					state.estimated_max_leadtime = max_leadtime;
					for (int64_t i = 0; i <= max_leadtime; i++) {
						state.estimated_leadtime_probs[i] = 0.0;
						state.past_leadtimes.push_back(0);
					}
					state.estimated_leadtime_probs[state.estimated_max_leadtime] = 1.0;
					state.censoredLeadtime = true;
					state.collectStatistics = true;
					state.orders_received = 0;
				}			
			}
			else {
				state.p = rng.genUniform() * (max_p - min_p) + min_p;
				state.min_leadtime = static_cast<int64_t>(std::floor(rng.genUniform() * (max_leadtime - min_leadtime + 1))) + min_leadtime;
				if (train_stochastic_leadtimes)
					state.max_leadtime = static_cast<int64_t>(std::floor(rng.genUniform() * (max_leadtime - state.min_leadtime + 1))) + state.min_leadtime;
				else
					state.max_leadtime = state.min_leadtime;
				leadtime_true_probs = SampleLeadTimeDistribution(rng, state.min_leadtime, state.max_leadtime);
				state.estimated_leadtime_probs = leadtime_true_probs;
				state.estimated_max_leadtime = state.max_leadtime;
				state.estimated_min_leadtime = state.min_leadtime;
				state.cycle_length = (int64_t)1;
				if (train_cyclic_demand)
					state.cycle_length += static_cast<int64_t>(std::floor(rng.genUniform() * max_num_cycles));
				mean_true_demand.reserve(state.cycle_length);
				stdev_true_demand.reserve(state.cycle_length);
				state.demand_cycles.reserve(state.cycle_length);
				for (int64_t i = 0; i < state.cycle_length; i++) {
					state.demand_cycles.push_back(i);
					double mean = rng.genUniform() * (max_demand - min_demand) + min_demand;
					mean_true_demand.push_back(mean);
					double min_var = DiscreteDist::LeastVarianceRequiredForAERFit(mean);
					double min_std = std::sqrt(min_var);
					double st_dev = rng.genUniform() * (mean * 2.0 - min_std) + min_std;
					stdev_true_demand.push_back(st_dev);
					if (state.cycle_length == 1) {
						if (state.estimated_max_leadtime == state.estimated_min_leadtime && state.estimated_max_leadtime == 6) {
							double a = (st_dev / mean) * (st_dev / mean) - 1 / mean;
							if (a > 1)
							{
								std::cout << state.p << "  " << mean << std::endl;
							}							
						}
					}
				}
				state.mean_cycle_demand = mean_true_demand;
				state.std_cycle_demand = stdev_true_demand;
				state.randomYield = false;
				if (train_random_yield) {
					if (rng.genUniform() < 0.5) {
						state.randomYield = true;
						if (state.randomYield) {
							if (rng.genUniform() < 0.5)
								state.yield_when_realized = true;
							else
								state.yield_when_realized = false;
						}
					}
				}
			}

			if (state.min_leadtime == state.max_leadtime) {
				state.stochasticLeadtimes = false;
				state.order_crossover = false;
			}
			else {
				state.stochasticLeadtimes = true;
				if (!evaluate) {
					if (rng.genUniform() < 0.5)
						state.order_crossover = false;
					else 
						state.order_crossover = true;
				}
				else {
					state.order_crossover = order_crossover;
				}
				std::vector<double> dummy_prob_vec(max_leadtime + 1, 1.0);
				state.cumulative_leadtime_probs = dummy_prob_vec;
				if (state.order_crossover) {
					double total_prob_v1 = 1.0;
					for (int64_t i = 0; i < state.max_leadtime; i++) {
						state.cumulative_leadtime_probs[i] = leadtime_true_probs[i] / total_prob_v1;
						total_prob_v1 -= leadtime_true_probs[i];
					}
				}
				else {
					double total_prob_v1 = 0.0;
					if (evaluate) {
						for (int64_t i = 0; i < state.max_leadtime; i++) {
							state.cumulative_leadtime_probs[i] = non_crossing_leadtime_rv_probs[i] + total_prob_v1;
							total_prob_v1 += non_crossing_leadtime_rv_probs[i];
						}
					}
					else {
						for (int64_t i = 0; i < state.max_leadtime; i++) {
							state.cumulative_leadtime_probs[i] = leadtime_true_probs[i] + total_prob_v1;
							total_prob_v1 += leadtime_true_probs[i];
						}
						double total_probs = 0.0;
						for (int64_t i = state.min_leadtime; i <= state.max_leadtime; i++) {
							double prob = 1.0;
							for (int64_t j = state.min_leadtime; j < i; j++) {
								prob *= std::max(0.0, 1.0 - state.cumulative_leadtime_probs[j]);
							}
							prob *= std::max(0.0, state.cumulative_leadtime_probs[i]);
							state.estimated_leadtime_probs[i] = prob;
							total_probs += prob;
						}
						// Normalize the probabilities if the total differs from 1
						if (std::abs(total_probs - 1.0) >= 1e-8) {
							for (double& prob : state.estimated_leadtime_probs) {
								prob /= total_probs;
							}
						}

						double total_prob_v2 = 0.0;
						for (const auto& prob : state.estimated_leadtime_probs)
						{
							if (prob < 0.0)
								throw DynaPlex::Error("Initiate state: non-crossover lead time probability is negative.");
							total_prob_v2 += prob;
						}
						if (std::abs(total_prob_v2 - 1.0) >= 1e-8)
							throw DynaPlex::Error("Initiate state: non-crossover total lead time probability should be 1.0.");
					}
				}
			}

			auto queue = Queue<int64_t>{}; //queue for state vector
			queue.reserve(max_leadtime + 1);
			queue.push_back(0);
			for (int64_t i = 1; i < max_leadtime; i++)
			{
				queue.push_back(0);
			}
			state.cat = StateCategory::AwaitAction();
			state.state_vector = queue;
			state.total_inv = queue.sum();

			std::vector<std::vector<double>> true_demand_probs;
			true_demand_probs.reserve(state.cycle_length);
			state.min_true_demand.reserve(state.cycle_length);
			state.cumulativePMFs.reserve(state.cycle_length);
			for (int64_t i = 0; i < state.cycle_length; i++) {
				DynaPlex::DiscreteDist state_demand_dist = DiscreteDist::GetAdanEenigeResingDist(mean_true_demand[i], stdev_true_demand[i]);
				state.min_true_demand.push_back(state_demand_dist.Min());
				std::vector<double> probs;
				probs.reserve(state_demand_dist.DistinctValueCount());
				std::vector<double> cumul_probs;
				cumul_probs.reserve(state_demand_dist.DistinctValueCount());
				double sum = 0.0;
				for (const auto& [qty, prob] : state_demand_dist) {
					probs.push_back(prob);
					sum += prob;
					cumul_probs.push_back(sum);
				}
				true_demand_probs.push_back(probs);
				state.cumulativePMFs.push_back(cumul_probs);
			}

			if (state.collectStatistics) {
				state.cycle_probs.reserve(state.cycle_length);
				state.cycle_min_demand.reserve(state.cycle_length);
				if (state.censoredDemand) {
					int64_t edge_min = edge_dist.Min();
					std::vector<double> edge_probs;
					edge_probs.reserve(edge_dist.DistinctValueCount());
					for (const auto& [qty, prob] : edge_dist) {
						edge_probs.push_back(prob);
					}
					for (int64_t i = 0; i < state.cycle_length; i++) {
						state.cycle_probs.push_back(edge_probs);
						state.cycle_min_demand.push_back(edge_min);
					}
				}
				else {
					for (int64_t i = 0; i < state.cycle_length; i++) {
						state.cycle_probs.push_back(true_demand_probs[i]);
						state.cycle_min_demand.push_back(state.min_true_demand[i]);
					}
				}
			}

			state.MaxOrderSize_Limit = 0;
			std::vector<double> probs_vec(state.estimated_leadtime_probs.begin() + state.estimated_min_leadtime, state.estimated_leadtime_probs.begin() + state.estimated_max_leadtime + 1);
			state.cycle_MaxOrderSize.reserve(state.cycle_length);
			state.cycle_MaxSystemInv.reserve(state.cycle_length);
			int64_t possible_leadtimes = state.estimated_max_leadtime - state.estimated_min_leadtime + 1;
			for (int64_t i = 0; i < state.cycle_length; i++) {
				std::vector<DiscreteDist> dist_vec;
				dist_vec.reserve(possible_leadtimes);
				std::vector<DiscreteDist> dist_vec_over_leadtime;
				dist_vec_over_leadtime.reserve(possible_leadtimes);
				for (int64_t j = state.estimated_min_leadtime; j <= state.estimated_max_leadtime; j++)
				{	
					auto DemOverLeadtime = DiscreteDist::GetZeroDist();
					for (int64_t k = 0; k < j; k++) {
						int64_t cyclePeriod = (i + k) % state.cycle_length;
						DynaPlex::DiscreteDist dist_over_lt = state.censoredDemand ? edge_dist : DiscreteDist::GetCustomDist(true_demand_probs[cyclePeriod], state.min_true_demand[cyclePeriod]);
						DemOverLeadtime = DemOverLeadtime.Add(dist_over_lt);
					}
					int64_t cyclePeriod_on_leadtime = (i + j) % state.cycle_length;
					DynaPlex::DiscreteDist dist_on_leadtime = state.censoredDemand ? edge_dist : DiscreteDist::GetCustomDist(true_demand_probs[cyclePeriod_on_leadtime], state.min_true_demand[cyclePeriod_on_leadtime]);
					DemOverLeadtime = DemOverLeadtime.Add(dist_on_leadtime);
					dist_vec.push_back(dist_on_leadtime);
					dist_vec_over_leadtime.push_back(DemOverLeadtime);
				}
				auto DummyDemOverLeadtime = DiscreteDist::MultipleMix(dist_vec_over_leadtime, probs_vec);
				auto DummyDemOnLeadtime = DiscreteDist::MultipleMix(dist_vec, probs_vec);
				int64_t OrderSize = std::min(DummyDemOnLeadtime.Fractile(state.p / (state.p + h)), MaxOrderSize);
				state.MaxOrderSize_Limit = std::max(state.MaxOrderSize_Limit, OrderSize);
				state.cycle_MaxOrderSize.push_back(OrderSize);
				state.cycle_MaxSystemInv.push_back(std::min(MaxSystemInv, DummyDemOverLeadtime.Fractile(state.p / (state.p + h))));
			}
			state.MaxOrderSize = state.cycle_MaxOrderSize[state.period];
			state.MaxOrderSize_Limit = state.censoredDemand ? MaxOrderSize : std::min(MaxOrderSize, state.MaxOrderSize_Limit);		
			state.MaxSystemInv = state.cycle_MaxSystemInv[state.period];
			state.OrderConstraint = std::max(static_cast<int64_t>(0), std::min(state.MaxSystemInv - state.total_inv, state.MaxOrderSize));

			if (train_random_yield) {
				if (state.randomYield) {
					auto dummy_queue = Queue<int64_t>{};
					dummy_queue.reserve(max_leadtime);
					for (int64_t i = 1; i < max_leadtime; i++)
					{
						dummy_queue.push_back(queue.at(i));
					}
					if (!state.yield_when_realized)
						state.pipeline_vector = dummy_queue;
					if (state.order_crossover)
						state.dummy_pipeline_vector = dummy_queue;

					if (!evaluate) {
						double min_yield_init = min_randomYield + (1.0 - min_randomYield) * rng.genUniform();
						int64_t MaxOrderWithYield = static_cast<int64_t>(std::ceil((double)state.MaxOrderSize_Limit * (1 / min_yield_init)));
						state.MaxOrderSize_Limit = std::min(MaxOrderSize, MaxOrderWithYield);
						state.random_yield_features.reserve(state.MaxOrderSize_Limit + 1);
						state.random_yield_features.push_back(0.0);
						if (state.order_crossover && state.yield_when_realized) {
							state.random_yield_probs_crossover.reserve(state.MaxOrderSize_Limit + 1);
							state.random_yield_probs_crossover.push_back(0.0);
						}
						else {
							state.random_yield_probs.reserve(state.MaxOrderSize_Limit + 1);
							state.random_yield_probs.push_back({ 0.0 });
						}

						double rand = rng.genUniform();
						if (rand < 0.33) {
							for (int64_t i = 1; i <= state.MaxOrderSize_Limit; i++)
							{
								if (state.order_crossover && state.yield_when_realized) {
									state.random_yield_probs_crossover.push_back(min_yield_init);
								}
								else {
									auto Dist = DiscreteDist::GetBinomialDist(i, min_yield_init);
									std::vector<double> probs(i + 1, 0.0);
									probs[i] = Dist.ProbabilityAt(i);
									for (int64_t j = i - 1; j >= 0; j--) {
										probs[j] = Dist.ProbabilityAt(j) + probs[j + 1];
									}
									state.random_yield_probs.push_back(probs);
								}
								state.random_yield_features.push_back(min_yield_init);
							}
						}
						else {
							double max_yield = 1.0;
							double min_mean = 0.0;
							for (int64_t i = 1; i <= state.MaxOrderSize_Limit; i++)
							{
								double yield = max_yield;
								int64_t count = 0;
								while (count < 10) {
									double dummy_yield = min_yield_init + (max_yield - min_yield_init) * rng.genUniform();
									if (i * dummy_yield * (1.0 - dummy_yield) >= min_mean) {
										min_mean = i * dummy_yield * (1.0 - dummy_yield);
										yield = dummy_yield;
										max_yield = yield;
										break;
									}
									count++;
								}
								if (state.order_crossover && state.yield_when_realized) {
									state.random_yield_probs_crossover.push_back(yield);
								}
								else {
									auto Dist = DiscreteDist::GetBinomialDist(i, yield);
									std::vector<double> probs(i + 1, 0.0);
									probs[i] = Dist.ProbabilityAt(i);
									for (int64_t j = i - 1; j >= 0; j--) {
										probs[j] = Dist.ProbabilityAt(j) + probs[j + 1];
									}
									state.random_yield_probs.push_back(probs);
								}
								state.random_yield_features.push_back(yield);
							}
						}
						for (int64_t i = 0; i < state.cycle_length; i++) {
							bool OrderSizeFound = false;
							for (int64_t j = state.cycle_MaxOrderSize[i]; j <= state.MaxOrderSize_Limit; j++) {
								if (static_cast<int64_t>(std::floor(j * state.random_yield_features[j])) >= state.cycle_MaxOrderSize[i]) {
									state.cycle_MaxOrderSize[i] = j;
									OrderSizeFound = true;
									break;
								}
							}
							if (!OrderSizeFound)
								state.cycle_MaxOrderSize[i] = state.MaxOrderSize_Limit;
						}
						state.MaxOrderSize = state.cycle_MaxOrderSize[state.period];
					}
					else {
						if (state.order_crossover && !state.yield_when_realized) 
							state.received_orders_vector = dummy_queue;

						state.random_yield_features.reserve(MaxOrderSize + 1);
						state.random_yield_features.push_back(0.0);

						if (randomYield_case == 0) {
							throw DynaPlex::Error("Initiate state for evaluation: randomYield_case == 0 not supported yet.");
						}
						else {
							if (state.order_crossover && state.yield_when_realized) {
								state.random_yield_probs_crossover.reserve(MaxOrderSize + 1);
								state.random_yield_probs_crossover.push_back(0.0);
							}

							if (randomYield_case == 1) {
								if (state.order_crossover && state.yield_when_realized) {
									for (int64_t i = 1; i <= MaxOrderSize; i++) {
										state.random_yield_probs_crossover.push_back(min_yield);
									}
								}
								if (!state.censoredRandomYield) {
									for (int64_t i = 1; i <= MaxOrderSize; i++) {
										state.random_yield_features.push_back(min_yield);
									}
								}
							}
							else {
								if ((state.order_crossover && state.yield_when_realized) || !state.censoredRandomYield) {
									int64_t n = 10000;
									for (int64_t i = 1; i <= MaxOrderSize; i++) {
										std::vector<int64_t> z_vec;
										z_vec.reserve(n);
										for (int64_t j = 0; j < n; j++) {
											double random_value = rng.genUniform();
											int64_t z = random_yield_dist.GetSampleFromProb(random_value);
											z_vec.push_back(z);
										}

										std::vector<int64_t> realized_order(i + 1, 0);
										for (int64_t j = 0; j < n; j++) {
											int64_t order = i;
											if (randomYield_case == 2) {
												order = std::min(i, z_vec[j]);
											}
											else if (randomYield_case == 3) {
												double pow_result = std::pow(static_cast<double>(z_vec[j]), static_cast<double>(p_var));
												int64_t order_received = static_cast<int64_t>(std::ceil(i * z_vec[j] / (i + alpha_var * pow_result)));
												order = std::min(i, order_received);
											}
											else if (randomYield_case == 4) {
												int64_t order_received = static_cast<int64_t>(std::ceil(i * k_var / (i + z_vec[j])));
												order = std::min(i, order_received);
											}
											realized_order[order]++;
										}
										double mean = 0.0;
										for (int64_t j = 0; j <= i; j++) {
											mean += j * realized_order[j] / n;
										}
										if (state.order_crossover && state.yield_when_realized)
											state.random_yield_probs_crossover.push_back(mean / i);
										if (!state.censoredRandomYield)
											state.random_yield_features.push_back(mean / i);
									}
								}
							}
						}
						if (state.censoredRandomYield) {
							state.random_yield_statistics.reserve(MaxOrderSize + 1);
							state.random_yield_statistics.push_back({ static_cast<int64_t>(0), static_cast<int64_t>(0) });
							for (int64_t i = 1; i <= MaxOrderSize; i++) {
								state.random_yield_statistics.push_back({ static_cast<int64_t>(0), static_cast<int64_t>(0) });
								state.random_yield_features.push_back(1.0);
							}
						}
						else {
							for (int64_t i = 0; i < state.cycle_length; i++) {
								bool OrderSizeFound = false;
								for (int64_t j = state.cycle_MaxOrderSize[i]; j <= MaxOrderSize; j++) {
									if (static_cast<int64_t>(std::floor(j * state.random_yield_features[j])) >= state.cycle_MaxOrderSize[i]) {
										state.cycle_MaxOrderSize[i] = j;
										OrderSizeFound = true;
										break;
									}
								}
								if (!OrderSizeFound)
									state.cycle_MaxOrderSize[i] = MaxOrderSize;
								if (state.cycle_MaxOrderSize[i] > state.MaxOrderSize_Limit)
									state.MaxOrderSize_Limit = state.cycle_MaxOrderSize[i];
							}
							state.MaxOrderSize = state.cycle_MaxOrderSize[state.period];
						}
					}
					double expected_total_inv = static_cast<double>(state.state_vector.front());
					for (int64_t i = 1; i < max_leadtime; i++) {
						int64_t expected_order = state.state_vector.at(i);
						if (expected_order > 0) {
							if (state.stochasticLeadtimes && state.order_crossover) {
								expected_total_inv += expected_order * state.random_yield_features[state.dummy_pipeline_vector.at(i - 1)];
							}
							else {
								expected_total_inv += expected_order * state.random_yield_features[expected_order];
							}
						}
					}
					int64_t effective_total_inv = static_cast<int64_t>(std::floor(expected_total_inv));
					state.OrderConstraint = std::max(static_cast<int64_t>(0), std::min(state.MaxSystemInv - effective_total_inv, state.MaxOrderSize));
				}
				else {
					state.random_yield_features.reserve(state.MaxOrderSize_Limit + 1);
					state.random_yield_features.push_back(0.0);
					for (int64_t i = 1; i <= state.MaxOrderSize_Limit; i++) {
						state.random_yield_features.push_back(1.0);
					}
				}
				if (include_all_features) {
					state.random_yield_nn_features.reserve(MaxOrderSize + 1);
					state.random_yield_nn_features.push_back(0.0);
					for (int64_t i = 1; i <= state.OrderConstraint; i++) {
						state.random_yield_nn_features.push_back(state.random_yield_features[i]);
					}
					for (int64_t i = state.OrderConstraint + 1; i <= MaxOrderSize; i++) {
						state.random_yield_nn_features.push_back(0.0);
					}						
				}
				else {			
					state.random_yield_nn_features.reserve(randomYield_features_size);
					double size = static_cast<double>(state.OrderConstraint) / randomYield_features_size;
					int64_t currentIndex = 0;
					for (int64_t i = 0; i < randomYield_features_size; ++i) {
						if (i < state.OrderConstraint % randomYield_features_size) 
							currentIndex += static_cast<int64_t>(std::ceil(size));					
						else 
							currentIndex += static_cast<int64_t>(std::floor(size));
						state.random_yield_nn_features.push_back(state.random_yield_features[currentIndex]);
					}
				}
			}
			return state;
		}

		std::vector<double> MDP::SampleLeadTimeDistribution(RNG& rng, int64_t min_lt, int64_t max_lt) const {
			int64_t possible_leadtimes = max_lt - min_lt + 1;
			std::vector<double> dummy_leadtime_probs(max_leadtime + 1, 0.0);
			std::string dist_type = "none";
			double total_probs = 0.0;

			if (possible_leadtimes == 1) {
				dummy_leadtime_probs[min_lt] = 1.0;
				total_probs = 1.0;
				dist_type = "Deterministic";
			}
			else if (rng.genUniform() < 0.33) {
				double prob = 1.0 / possible_leadtimes;
				for (int64_t i = min_lt; i <= max_lt; i++) {
					dummy_leadtime_probs[i] = prob;
					total_probs += prob;
				}
				dist_type = "Uniform";
			}
			else {
				double mean = (double)(max_lt + min_lt) / 2.0;
				if (rng.genUniform() < 0.5 && mean > 2.0) {
					double min_var = DiscreteDist::LeastVarianceRequiredForAERFit(mean);
					double min_std = std::sqrt(min_var);
					double stdev = rng.genUniform() * (mean * 2.0 - min_std) + min_std;
					DynaPlex::DiscreteDist dist = DiscreteDist::GetAdanEenigeResingDist(mean, stdev);
					for (int64_t i = min_lt; i <= max_lt; i++) {
						double lt_prob = dist.ProbabilityAt(i);
						dummy_leadtime_probs[i] = lt_prob;
						total_probs += lt_prob;
					}
					dist_type = "AER";
				}
				else {
					double remaining_probability = 1.0;
					for (int64_t i = min_lt; i < max_lt; i++) {
						double lt_prob = std::max(0.0, rng.genUniform() * remaining_probability);
						dummy_leadtime_probs[i] = lt_prob;
						total_probs += lt_prob;
						remaining_probability -= lt_prob;
					}
					double lt_prob = std::max(0.0, remaining_probability);
					dummy_leadtime_probs[max_lt] = lt_prob;
					total_probs += lt_prob;
					dist_type = "RAND";
				}
			}
			// Normalize the probabilities if the total differs from 1
			if (std::abs(total_probs - 1.0) >= 1e-8) {
				for (double& prob : dummy_leadtime_probs) {
					prob /= total_probs;
				}
			}
			if (std::any_of(dummy_leadtime_probs.begin(), dummy_leadtime_probs.end(), [](double num) { return std::isnan(num); })) {
				throw DynaPlex::Error("Initiate state: sample lead time: probability value is Nan. Dist: " + dist_type);
			}

			double total_prob_v2 = 0.0;
			for (const auto& prob : dummy_leadtime_probs)
			{
				if (prob < 0.0)
					throw DynaPlex::Error("Initiate state: sample lead time: lead time probability is negative. Dist: " + dist_type);
				total_prob_v2 += prob;
			}
			if (std::abs(total_prob_v2 - 1.0) >= 1e-8)
				throw DynaPlex::Error("Initiate state - sample lead time: total lead time probabilities should sum up to 1.0. Dist: " + dist_type);

			return dummy_leadtime_probs;
		}

		MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("p", state.p);
			vars.Get("state_vector", state.state_vector);
			vars.Get("mean_cycle_demand", state.mean_cycle_demand);
			vars.Get("std_cycle_demand", state.std_cycle_demand);
			vars.Get("demand", state.demand);
			if (train_cyclic_demand) {
				vars.Get("period", state.period);
				vars.Get("cycle_length", state.cycle_length);
			}
			if (train_stochastic_leadtimes) {
				vars.Get("order_crossover", state.order_crossover);
				vars.Get("estimated_leadtime_probs", state.estimated_leadtime_probs);
			}
			else {
				vars.Get("min_leadtime", state.min_leadtime);
			}
			if (train_random_yield) {
				vars.Get("OrderConstraint", state.OrderConstraint);
				vars.Get("random_yield_nn_features", state.random_yield_nn_features);
			}

			return state;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("p", p);
			vars.Add("state_vector", state_vector);
			vars.Add("mean_cycle_demand", mean_cycle_demand);
			vars.Add("std_cycle_demand", std_cycle_demand);
			vars.Add("demand", demand);
			vars.Add("period", period);
			vars.Add("cycle_length", cycle_length);
			vars.Add("order_crossover", order_crossover);
			vars.Add("estimated_leadtime_probs", estimated_leadtime_probs);
			vars.Add("min_leadtime", min_leadtime);
			vars.Add("OrderConstraint", OrderConstraint);
			vars.Add("random_yield_nn_features", random_yield_nn_features);

			return vars;
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}

		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
				/*=id though which the MDP will be retrievable*/ "Zero_Shot_Lost_Sales_Inventory_Control",
				/*description*/ "Lost sales problem with cyclic censored demand, censored stochastic lead times and censored random yields.)",
				/*reference to passed registry*/registry); 
		}

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<BaseStockPolicy>("base_stock",
				"Oracle base-stock policy with parameter S.");
			registry.Register<CappedBaseStockPolicy>("capped_base_stock",
				"Oracle capped base-stock policy with parameters S and r.");
			registry.Register<GreedyCappedBaseStockPolicy>("greedy_capped_base_stock",
				"Capped base-stock policy with suboptimal S and r.");
			registry.Register<ConstantOrderPolicy>("constant_order",
				"Constant order policy with parameter co_level.");
		}
	}
}

