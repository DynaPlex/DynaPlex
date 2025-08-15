#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"

namespace DynaPlex::Models {
	namespace multi_item_sla
	{
		class MDP
		{
		public:
			std::vector<int64_t> leadTimes;
			double penaltyCost;
			double backOrderCost;
			double sendBackCost;
			double unavoidableCostPerPeriod;
			std::vector<double> holdingCosts;
			std::vector<double> demandRates;
			double totalDemandRate;
			std::vector<DynaPlex::DiscreteDist> demand_distributions;

			bool sendBackUnits;
			int64_t reviewHorizon;
			double aggregateTargetFillRate;
			int64_t numberOfItems;
			int64_t benchmarkAction;
			int64_t totalActions;
			std::vector<std::vector<int64_t>> baseStockLevels;
			std::vector<int64_t> highDemandVariance;

			struct State {
				DynaPlex::StateCategory cat;
				
				std::vector<int64_t> aggregate_vector;
				std::vector<Queue<int64_t>> state_vector;
				std::vector<int64_t> inventory_position;
				int64_t ObservedDemand;
				int64_t TimeRemaining;
				int64_t CumulativeStockouts;
				double CSPerReviewPeriod; // cumulative stockouts
				double CESPerReviewPeriod; // excessive stockouts
				double AggregateFillRate;
				double AFRPerReviewPeriod;
				int64_t ChangeInPolicy;
				double CIPPerReviewPeriod; // change in policy
				double SPPerReviewPeriod; // success percentage
				int64_t LastPolicy;
				int64_t NumReviewPeriodPassed;
				std::vector<bool> AllowedActions;
				//double HoldingCosts;
				//double HoldingCostsPerReviewPeriod;
				std::vector<int64_t> policyChange;

				DynaPlex::VarGroup ToVarGroup() const;
			};
  
			using Event = std::vector<int64_t>;

			std::vector<double> ReturnUsefulStatistics(const State&) const;
			void ResetHiddenStateVariables(State& state, DynaPlex::RNG&) const;
			void SetAllowedActions(State& state) const;
			int64_t GetH(const State&) const;

			//Remainder of the DynaPlex API:
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG&) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State&, int64_t action) const;
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			explicit MDP(const DynaPlex::VarGroup&);
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
		};
	}
}