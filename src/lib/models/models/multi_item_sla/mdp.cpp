#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"

namespace DynaPlex::Models {
	namespace multi_item_sla /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;		
			vars.Add("valid_actions", totalActions);
			
			return vars;
		}

		int64_t MDP::GetH(const State& state) const
		{
			return reviewHorizon + state.TimeRemaining + 1;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("numberOfItems", numberOfItems);
			config.Get("leadTimes", leadTimes);
			config.Get("holdingCosts", holdingCosts);
			config.Get("demandRates", demandRates);
			config.Get("highDemandVariance", highDemandVariance);
			config.Get("totalDemandRate", totalDemandRate);
			config.Get("sendBackUnits", sendBackUnits);
			config.Get("backOrderCost", backOrderCost);

			std::vector<int64_t> concatBaseStockLevels;

			config.Get("aggregateTargetFillRate", aggregateTargetFillRate);
			config.GetOrDefault("unavoidableCostPerPeriod", unavoidableCostPerPeriod, 0.0);
			config.Get("reviewHorizon", reviewHorizon);
			config.Get("totalActions", totalActions);
			config.GetOrDefault("benchmarkAction", benchmarkAction, static_cast<int64_t>(std::floor((double)totalActions / 2.0)));
			config.GetOrDefault("penaltyCost", penaltyCost, 100.0);
			config.Get("concatBaseStockLevels", concatBaseStockLevels);
			sendBackCost = 0.0;

			if (benchmarkAction >= totalActions)
				benchmarkAction = totalActions - 1;

			demand_distributions.reserve(numberOfItems);
			for (int64_t i = 0; i < numberOfItems; i++) {
				if (highDemandVariance[i] == 1)
					demand_distributions.push_back(DynaPlex::DiscreteDist::GetGeometricDist(demandRates[i]));
				else 
					demand_distributions.push_back(DynaPlex::DiscreteDist::GetPoissonDist(demandRates[i]));
			}

			baseStockLevels.reserve(totalActions);
			for (int64_t l = 0; l < totalActions; l++)
			{
				std::vector<int64_t> subset(concatBaseStockLevels.begin() + l * numberOfItems, concatBaseStockLevels.begin() + (l + 1) * numberOfItems);
				baseStockLevels.push_back(subset);
			}
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const
		{
			return state.AllowedActions[action];
		}

		void MDP::SetAllowedActions(State& state) const
		{			
			state.AllowedActions.resize(totalActions, false);
			bool noneAllowed = true;
			for (int64_t j = 1; j < totalActions - 1; ++j)
			{
				const auto& bs_levels = baseStockLevels[j - 1];
				const auto& bs_levels_high = baseStockLevels[j];
				for (int64_t i = 0; i < numberOfItems; ++i)
				{
					if (bs_levels[i] > state.inventory_position[i] && bs_levels[i] < bs_levels_high[i]) {
						state.AllowedActions[j - 1] = true;
						noneAllowed = false;
						break;
					}
				}
			}

			const auto& bs_levels = baseStockLevels[totalActions - 1];
			for (int64_t i = 0; i < numberOfItems; ++i)
			{
				if (bs_levels[i] > state.inventory_position[i]) {
					state.AllowedActions[totalActions - 1] = true;
					noneAllowed = false;
					break;
				}
			}

			if (noneAllowed)
				state.AllowedActions[benchmarkAction] = true;
		}

		double MDP::ModifyStateWithAction(State& state, int64_t action) const
		{
			double cost = 0.0;	
			const std::vector<int64_t> New_BS_Levels = baseStockLevels[action];
			for (int64_t i = 0; i < numberOfItems; i++)
			{
				const int64_t toOrder = New_BS_Levels[i] - state.inventory_position[i];
				if (toOrder >= 0) {
					if (leadTimes[i] == 0)
						state.state_vector[i].front() += toOrder;
					else
						state.state_vector[i].push_back(toOrder);
					state.inventory_position[i] += toOrder;
				}
				else {
					if (leadTimes[i] != 0)
						state.state_vector[i].push_back(0);
					if (sendBackUnits && state.state_vector[i].front() > 0) {
						int64_t numSendBackUnits = std::min(-toOrder, state.state_vector[i].front());
						state.state_vector[i].front() -= numSendBackUnits;
						state.inventory_position[i] -= numSendBackUnits;
						cost += sendBackCost * numSendBackUnits;
					}
				}
			}

			if (action != state.LastPolicy) {
				state.LastPolicy = action;
				state.ChangeInPolicy++;
				state.policyChange[reviewHorizon - state.TimeRemaining]++;
			}

			state.cat = StateCategory::AwaitEvent();

			return cost;
		}

		MDP::Event MDP::GetEvent(RNG& rng) const 
		{
			std::vector<int64_t> events;
			events.reserve(numberOfItems);
			for (int64_t i = 0; i < numberOfItems; i++)
			{
				events.push_back(demand_distributions[i].GetSample(rng));
			}

			return events;
		}

		double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const
		{
			state.cat = StateCategory::AwaitAction();
			double cost = 0.0;
			state.TimeRemaining--;
			state.aggregate_vector.clear();

			for (int64_t i = 0; i < numberOfItems; i++)
			{
				auto& currentState = state.state_vector[i];
				int64_t onHand = currentState.pop_front();
				const int64_t demand = event[i];
				int64_t& invPos = state.inventory_position[i];
				invPos -= demand;
				state.ObservedDemand += demand;

				if (onHand >= demand)
				{
					onHand -= demand;
					cost += onHand * holdingCosts[i];
				}
				else
				{
					const int64_t numStockout = demand - (onHand > 0 ? onHand : 0);
					state.CumulativeStockouts += numStockout;
					cost += backOrderCost * numStockout;
					onHand -= demand; 
				}

				if (leadTimes[i] == 0)
					currentState.push_back(onHand);
				else
					currentState.front() += onHand;

				state.aggregate_vector.insert(state.aggregate_vector.end(), currentState.begin(), currentState.end());
			}
			//state.HoldingCosts += cost;

			if (state.ObservedDemand > 0)
				state.AggregateFillRate = static_cast<double>(state.ObservedDemand - state.CumulativeStockouts) / (static_cast<double>(state.ObservedDemand));
			
			if (state.TimeRemaining == 0)
			{
				state.NumReviewPeriodPassed++;
				const int64_t shouldSatisfy = static_cast<int64_t>(std::ceil(aggregateTargetFillRate * state.ObservedDemand));
				const int64_t maxStockoutsAllowed = state.ObservedDemand - shouldSatisfy;
				int64_t exceededStockouts = state.CumulativeStockouts - maxStockoutsAllowed;
				if (exceededStockouts > 0)
				{
					cost += exceededStockouts * penaltyCost;
					state.CESPerReviewPeriod = (state.CESPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + (double)exceededStockouts) / (double)state.NumReviewPeriodPassed;
					state.SPPerReviewPeriod = (state.SPPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + 0.0) / (double)state.NumReviewPeriodPassed;
				}
				else {
					state.CESPerReviewPeriod = (state.CESPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + 0.0) / (double)state.NumReviewPeriodPassed;
					state.SPPerReviewPeriod = (state.SPPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + 1.0) / (double)state.NumReviewPeriodPassed;
				}
				state.AFRPerReviewPeriod = (state.AFRPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + state.AggregateFillRate) / (double)state.NumReviewPeriodPassed;
				state.CSPerReviewPeriod = (state.CSPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + (double)state.CumulativeStockouts) / (double)state.NumReviewPeriodPassed;
				state.CIPPerReviewPeriod = (state.CIPPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + (double)state.ChangeInPolicy) / (double)state.NumReviewPeriodPassed;
				//state.HoldingCostsPerReviewPeriod = (state.HoldingCostsPerReviewPeriod * (state.NumReviewPeriodPassed - 1) + state.HoldingCosts) / (double)state.NumReviewPeriodPassed;
				state.ChangeInPolicy = 0;
				state.ObservedDemand = 0;
				state.AggregateFillRate = 1.0;
				state.TimeRemaining = reviewHorizon;
				state.CumulativeStockouts = 0;
			}

			SetAllowedActions(state);
			return cost - unavoidableCostPerPeriod;
		}

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features) const {
			features.Add(state.aggregate_vector);
			if (state.TimeRemaining < reviewHorizon) {
				features.Add(state.AggregateFillRate);
				features.Add(static_cast<float>(state.CumulativeStockouts) / static_cast<float>(totalDemandRate * (reviewHorizon - state.TimeRemaining)));
				features.Add(static_cast<float>(state.ObservedDemand) / static_cast<float>(totalDemandRate * (reviewHorizon - state.TimeRemaining)));
			}
			else {
				features.Add(1.0);
				features.Add(0.0);
				features.Add(0.0);
			}
			features.Add(static_cast<float>(state.TimeRemaining) / static_cast<float>(reviewHorizon));
		}

		std::vector<double> MDP::ReturnUsefulStatistics(const State& state) const
		{
			std::vector<double> statistics;
			statistics.reserve(8);
			statistics.push_back(state.AFRPerReviewPeriod);
			statistics.push_back(state.CSPerReviewPeriod);
			statistics.push_back(state.CESPerReviewPeriod);
			statistics.push_back(state.CIPPerReviewPeriod);
			statistics.push_back(state.SPPerReviewPeriod);

			std::vector<int64_t> polChange(3, 0);
			int64_t inc = static_cast<int64_t>(floor((double)reviewHorizon / 3.0));
			for (int64_t i = 0; i < reviewHorizon; i++){
				if (i < inc)
					polChange[0] += state.policyChange[i];
				else if (i < 2 * inc)
					polChange[1] += state.policyChange[i];
				else
					polChange[2] += state.policyChange[i];
			}
			int64_t totalChange = polChange[0] + polChange[1] + polChange[2];
			for (int64_t i = 0; i < 3; i++) {
				double frac = 0.0;
				if (totalChange > 0)
					frac = static_cast<double>(polChange[i]) / totalChange;
				statistics.push_back(frac);
			}
			//statistics.push_back(state.HoldingCosts);
			return statistics;
		}

		void MDP::ResetHiddenStateVariables(State& state, RNG& rng) const
		{
			state.ObservedDemand = 0;
			state.AggregateFillRate = 1.0;
			state.TimeRemaining = reviewHorizon;
			state.CumulativeStockouts = 0;
			state.ChangeInPolicy = 0;
			state.NumReviewPeriodPassed = 0;
			state.policyChange.resize(reviewHorizon, 0);
			//state.HoldingCosts = 0.0;
		} 
		
		MDP::State MDP::GetInitialState() const
		{
			State state{};
			state.cat = StateCategory::AwaitAction();
			state.state_vector.reserve(numberOfItems);
			state.inventory_position.reserve(numberOfItems);
			int64_t aggregateVectorLength = 0;
			for (int64_t i = 0; i < numberOfItems; i++) {
				auto queue = Queue<int64_t>{}; 
				queue.reserve(leadTimes[i] + 1);
				queue.push_back(baseStockLevels[benchmarkAction][i]);//<- initial on-hand
				for (int64_t j = 0; j < leadTimes[i] - 1; j++)
				{
					queue.push_back(0);
				}
				state.state_vector.push_back(queue);
				state.inventory_position.push_back(baseStockLevels[benchmarkAction][i]);
				aggregateVectorLength += leadTimes[i] == 0 ? 1 : leadTimes[i];
			}
			state.aggregate_vector.reserve(aggregateVectorLength);
			for (int64_t i = 0; i < numberOfItems; i++) {
				state.aggregate_vector.insert(state.aggregate_vector.end(), state.state_vector[i].begin(), state.state_vector[i].end());
			}

			state.LastPolicy = benchmarkAction;
			state.ObservedDemand = 0;
			state.AggregateFillRate = 1.0;
			state.TimeRemaining = reviewHorizon;
			state.CumulativeStockouts = 0;
			state.ChangeInPolicy = 0;
			state.NumReviewPeriodPassed = 0;
			state.AFRPerReviewPeriod = 1.0;
			state.CSPerReviewPeriod = 0.0;
			state.CESPerReviewPeriod = 0.0;
			state.CIPPerReviewPeriod = 0.0;
			state.SPPerReviewPeriod = 1.0;
			state.AllowedActions.resize(totalActions, false);
			state.AllowedActions[benchmarkAction] = true;
			state.policyChange.resize(reviewHorizon, 0);
			//state.HoldingCosts = 0.0;

			return state;
		}
	
		MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("aggregate_vector", state.aggregate_vector);
			vars.Get("ObservedDemand", state.ObservedDemand);
			vars.Get("AggregateFillRate", state.AggregateFillRate);
			vars.Get("TimeRemaining", state.TimeRemaining);
			vars.Get("CumulativeStockouts", state.CumulativeStockouts);
			vars.Get("SPPerReviewPeriod", state.SPPerReviewPeriod);
			return state;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("aggregate_vector", aggregate_vector);
			vars.Add("ObservedDemand", ObservedDemand);
			vars.Add("AggregateFillRate", AggregateFillRate);
			vars.Add("TimeRemaining", TimeRemaining);
			vars.Add("CumulativeStockouts", CumulativeStockouts);
			vars.Add("SPPerReviewPeriod", SPPerReviewPeriod);
			return vars;
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}
		
		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<BaseStockPolicy>("base_stock",
				"Base-stock policy with parameter base_stock_level for all SKUs.");
			registry.Register<DynamicPolicy>("dynamic",
				"Dynamic base-stock policy for all SKUs.");
			registry.Register<GreedyDynamicPolicy>("greedy_dynamic",
				"Dynamic base-stock policy for all SKUs.");
		}
		
		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
				/*=id though which the MDP will be retrievable*/ "multi_item_sla",
				/*description*/ "Single location multi item inventory management under a service level agreement.",
				/*reference to passed registry*/registry); 
		}
	}
}