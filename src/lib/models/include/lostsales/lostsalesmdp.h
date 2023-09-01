#pragma once
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/rng.h"
#include "dynaplex/statetype.h"
#include "dynaplex/features.h"

namespace DynaPlex::Models {
	namespace LostSales /*keep this in line with id below*/
	{
		//forward declaration
		class BaseStockPolicy;
		
		class MDP
		{			
			friend class DynaPlex::Models::LostSales::BaseStockPolicy;
			const DynaPlex::VarGroup varGroup;

			double p, h;
			int64_t leadtime;
			int64_t MaxOrderSize;
			int64_t MaxSystemInv;
			DynaPlex::DiscreteDist demand_dist;
		public:
		

			using Event = int64_t;
			struct State {
				DynaPlex::StateType state_type;
				Queue<int64_t> state_vector;
				int64_t total_inv;			

				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;
					vars.Add("state_type", state_type);
					vars.Add("state_vector", state_vector);		
					vars.Add("total_inv", total_inv);
					return vars;
				}
			};
			DynaPlex::VarGroup GetStaticInfo() const;

			DynaPlex::StateType GetStateType(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;


			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;

			//something to extract features. 

			State GetInitialState() const;

			static BaseStockPolicy GetPolicy(const VarGroup&, std::shared_ptr<const MDP>);

			void GetFeatures(const State&, Features&) const;

			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

