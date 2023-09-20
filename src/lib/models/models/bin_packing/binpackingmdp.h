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
			const DynaPlex::VarGroup varGroup;
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
				State() = default;


				explicit State(const VarGroup& vars)
				{
					vars.Get("cat", cat);
					vars.Get("weight_vector", weight_vector);
					vars.Get("upcoming_weight", upcoming_weight);
				}

				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;
					vars.Add("cat", cat);
					vars.Add("weight_vector", weight_vector);
					vars.Add("upcoming_weight", upcoming_weight);
					return vars;
				}
			};
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState() const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

