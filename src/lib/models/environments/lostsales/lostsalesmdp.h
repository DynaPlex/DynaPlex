#pragma once
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/rng.h"
#include "dynaplex/statecategory.h"
#include "dynaplex/features.h"
#include "dynaplex/policyregistry.h"
namespace DynaPlex {
	template <typename MDPType>
	class PolicyRegistry; // Forward declaration
} // namespace DynaPlex

namespace DynaPlex::Models {
	namespace LostSales /*keep this in line with id below*/
	{		
		class MDP
		{			
			//to give this access to private members:
			friend class BaseStockPolicy;
			const DynaPlex::VarGroup varGroup;

			double p, h;
			int64_t leadtime;
			int64_t MaxOrderSize;
			int64_t MaxSystemInv;
			DynaPlex::DiscreteDist demand_dist;
		public:
		

			using Event = int64_t;
			struct State {
				DynaPlex::StateCategory cat;
				Queue<int64_t> state_vector;
				int64_t total_inv;			

				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;
					vars.Add("cat", cat);
					vars.Add("state_vector", state_vector);		
					vars.Add("total_inv", total_inv);
					return vars;
				}
			};
			DynaPlex::VarGroup GetStaticInfo() const;

			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;


			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;

			void GetFeatures(State&,DynaPlex::Features&);

			State GetInitialState() const;

			void RegisterPolicies(PolicyRegistry<MDP>& registry) const;

			void GetFeatures(const State&, Features&) const;

			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

