#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/jointdiscretedist.h"
#include "dynaplex/modelling/queue.h"
#include <map>

namespace DynaPlex::Models {
	namespace perishable_systems /*must be consistent everywhere for complete mdp definition and associated policies and states.*/
	{
		class MDP
		{
		public:
			//This is optional, if you want the discount_factor to be configurable.
			//The static MDP information returned by GetStaticMDP sets the actual discount_factor used by DynaPlex. 
			//if not provided, defaults to 1.0.  
			double discount_factor;

			//Other MDP variables:
			double p, h, c, o; //costs - penalty, holding, ordering, waste -
			int64_t LeadTime;
			int64_t ProductLife;
			double f, mu, cvr; //issuance policy ratio, mean demand, square root of variance over mean

			int64_t MaxSystemInv;
			DynaPlex::JointDiscreteDist demand_dist;
			std::vector<std::vector<int64_t>> demand_combination_holder;

			//A state is a struct (or class) that represents state information for the MDP:
			struct State {
				//State category keeps track of what should happen next to this state:
				// an event, an action, or maybe the MDP reached a final state. It will
				//typically be convenient to have this member variable in the state, as this will
				//make GetStateCategory() trivial to implement. 
				DynaPlex::StateCategory cat;

				//Other members depend on the MDP:
				Queue<int64_t> state_vector;

				//declaration; for definition see mdp.cpp:
				DynaPlex::VarGroup ToVarGroup() const;
				bool operator==(const State& other) const = default;
			};

			using Event = int64_t;

			//Remainder of the DynaPlex API:
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG&) const;
			std::vector<std::tuple<Event, double>> EventProbabilities() const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State&, int64_t action) const;
			//You may also define this with a parameter DynaPlex::RNG&, for random initial states:
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			//Enables all MDPs to be constructer in a uniform manner.
			explicit MDP(const DynaPlex::VarGroup&);
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;

		};
	}
}