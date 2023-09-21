#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"

//namespace DynaPlex::Erasure { template <typename MDPType> class PolicyRegistry; }// Forward declaration 

namespace DynaPlex::Models {
	namespace empty_example /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{		
		class MDP
		{			
		public:	
			double discount_factor;
			//any other mdp variables go here:
			 
			struct State {
				//using this is recommended:
				DynaPlex::StateCategory cat;
				DynaPlex::VarGroup ToVarGroup() const;
			};
			//Event may also be struct or class like.
			using Event = int64_t;

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

