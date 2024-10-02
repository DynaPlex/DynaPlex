#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/modelling/matrix.h"

//namespace DynaPlex::Erasure { template <typename MDPType> class PolicyRegistry; }// Forward declaration 

namespace DynaPlex::Models {
	namespace dual_sourcing /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{		
		class MDP
		{
		public:
			double discount_factor;
			//any other mdp variables go here:

			std::vector<std::vector<DynaPlex::DiscreteDist>> failureProbs_cm_{};//for demand probs CM
			std::vector<std::vector<DynaPlex::DiscreteDist>> failureProbs_am_{};//for demand probs AM

			std::vector<int64_t> Instance; //Instance no.
			std::vector<int64_t> reviewPeriod;//tau
			std::vector<int64_t> InstalledBase_;
			std::vector<double> meanFailureCM_;
			std::vector<double> varFailureCM_;
			std::vector<int64_t> leadtimeCM_;
			std::vector<double> piecePriceCM_;//c
			std::vector<double> orderCostsCM_;
			std::vector<int64_t> batchSizeCM;
			std::vector<double> meanFailureAM_;
			std::vector<double> varFailureAM_;
			std::vector<int64_t> leadtimeAM_;
			std::vector<double> piecePriceAM_;//c
			std::vector<double> maintenanceCosts_;
			std::vector<double> holdingCosts_;
			std::vector<double> backorderCosts_;
			std::vector<int64_t> sMax;//maximum inventory position

			int64_t inst_no;

			DynaPlex::DiscreteDist instance_probs;

			static constexpr bool sampleParams = false;//use constexpr since it is faster as it is evaluated during build
			DynaPlex::DiscreteDist two_probs;
			DynaPlex::DiscreteDist three_probs;
			DynaPlex::DiscreteDist four_probs;
			std::vector < std::vector < std::vector < std::vector<std::vector<double>>>>>failureProbs_cm{};//for demand probs CM
			std::vector < std::vector < std::vector < std::vector<std::vector<double>>>>> failureProbs_am{};//for demand probs AM

			//bool test;

			//diagnostics

			struct PartInfoState
			{
				int64_t operatingItems{};
				int64_t onHandStock{};
				int64_t InventoryPosition{};
				std::vector<int64_t> orderQuantities{};
				DynaPlex::VarGroup ToVarGroup() const;//since use nested struct in our state, we need to add this
				bool operator==(const PartInfoState& other) const = default;
			};

			 
			struct State {
				PartInfoState cmPart{};
				PartInfoState amPart{};

				//two additional parameters used only for the single index policy
				int64_t lastdemand;
				int64_t inventoryposition;

				//new for learning instance-robust master policy
				double meanFailureCM;
				double varFailureCM;
				int64_t leadtimeCM;
				double piecePriceCM;//c
				double orderCostsCM;
				double meanFailureAM;
				double varFailureAM;
				int64_t leadtimeAM;
				double piecePriceAM;//c
				double maintenanceCosts;
				double holdingCosts;
				double backorderCosts;
				int64_t InstalledBase;

				//below are helpers, not provided to policy
				int64_t ref_meanFailureCM;
				int64_t ref_meanFailureAM;
				int64_t ref_varToMeanRatio;

				int64_t Inst;//just for tracking, not provided to policy

				DynaPlex::StateCategory cat;

				DynaPlex::VarGroup ToVarGroup() const;
				//Defaulting this does not always work. It can be removed as only the exact solver would benefit from this. 
				bool operator==(const State& other) const = default;
			};
			//Event may also be struct or class like.
			struct Event {
				int64_t demand_cm;
				int64_t demand_am;
			};
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event& ) const;
			Event GetEvent(const State& state,DynaPlex::RNG& rng) const;
			//std::vector<std::tuple<Event,double>> EventProbabilities() const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState(DynaPlex::RNG& rng) const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);

			//helpers
			int64_t decisionSpace;
			std::vector<std::vector<int64_t>> actionMatrix{};//contains all action 
			int64_t GetSample(RNG& rng, std::vector<double> pmf) const;
			int64_t Fact(int64_t num) const;
			std::vector<double> GetNegBinPMF(long uBound, double r, double p) const;
			int64_t BinomialCoefficient(const double n, const int64_t k) const;
			std::vector<double> GetPoissonPMF(long uBound, double rate) const;
			int64_t findPossibleWays(int64_t coins[], int64_t n, int64_t sum) const;

		};
	}
}

