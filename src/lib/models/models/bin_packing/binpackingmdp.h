#pragma once
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/rng.h"
#include "dynaplex/statecategory.h"
#include "dynaplex/features.h"
#include "dynaplex/erasure/policyregistry.h"


//namespace DynaPlex::Erasure { template <typename MDPType> class PolicyRegistry; }// Forward declaration 

namespace DynaPlex::Models {
	namespace bin_packing /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{		
		class MDP
		{			
			double discount_factor;

			int64_t max_bin_size;
			int64_t number_of_bins;
			DynaPlex::DiscreteDist weight_dist;
		public:
			using Event = int64_t;
			struct State {
				DynaPlex::StateCategory cat;
				std::vector<int64_t> weight_vector;
				int64_t upcoming_weight;	

				DynaPlex::VarGroup ToVarGroup() const;
			};
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, DynaPlex::RNG&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState() const;
			State GetState(const VarGroup& vars) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

