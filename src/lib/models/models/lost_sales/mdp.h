#pragma once
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/rng.h"
#include "dynaplex/statecategory.h"
#include "dynaplex/features.h"
#include "dynaplex/erasure/policyregistry.h"

//namespace DynaPlex::Erasure { template <typename MDPType> class PolicyRegistry; }// Forward declaration 

namespace DynaPlex::Models {
	namespace lost_sales /*must be consistent everywhere for complete mdp definition and associated policies and states.*/
	{		
		class MDP
		{			
			//to give this access to private members:
			friend class BaseStockPolicy;
			//MDP variables:
			double p, h, discount_factor;
			int64_t leadtime;
			int64_t MaxOrderSize;
			int64_t MaxSystemInv;
			DynaPlex::DiscreteDist demand_dist;
		public:
			using Event = int64_t;

			struct State {
				//State variables:
				DynaPlex::StateCategory cat;
				Queue<int64_t> state_vector;
				int64_t total_inv;
				
				DynaPlex::VarGroup ToVarGroup() const;
			};
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&,const Event&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

