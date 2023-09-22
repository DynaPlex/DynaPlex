#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"

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
				//Defaulting this does not always work. It can be removed as only the exact solver might benefit from this. 
				bool operator==(const State& other) const = default;
				DynaPlex::VarGroup ToVarGroup() const;
			};
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event& ) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

