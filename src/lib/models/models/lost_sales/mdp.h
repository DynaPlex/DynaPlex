#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"

//namespace DynaPlex::Erasure { template <typename MDPType> class PolicyRegistry; }// Forward declaration 

namespace DynaPlex::Models {
	namespace lost_sales /*must be consistent everywhere for complete mdp definition and associated policies and states.*/
	{		
		class MDP
		{
		public:			
			//This is optional, if you want the discount_factor to be configurable.
			//The static MDP information returned by GetStaticMDP sets the actual discount_factor used by DynaPlex. 
			//if not provided, defaults to 1.0.  
			double  discount_factor;

			//Other MDP variables:
			double p, h;
			int64_t leadtime;
			int64_t MaxOrderSize;
			int64_t MaxSystemInv;
			DynaPlex::DiscreteDist demand_dist;
			//A state is a struct (or class) that represents state information for the MDP:
			struct State {
				//State category keeps track of what should happen next to this state:
				// an event, an action, or maybe the MDP reached a final state. It will
				//typically be convenient to have this member variable in the state, as this will
				//make GetStateCategory() trivial to implement. 
				DynaPlex::StateCategory cat;

				//Other members depend on the MDP:
				Queue<int64_t> state_vector;
				int64_t total_inv;
				
				//declaration; for definition see mdp.cpp:
				DynaPlex::VarGroup ToVarGroup() const;
			};

			//This may be changed as needed, e.g. into struct Event, as struct State above. 
			//When defining a custom event struct (instead of a primitive like int64_t), it is _optional_
			// (typically not needed) to also add:
			// -	Event GetEvent(const VarGroup&) const;  (on MDP)
			// -    To expose DynaPlex::VarGroup ToVarGroup() const		  
			using Event = int64_t;


			//Remainder of the DynaPlex API:
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&,const Event&) const;
			Event GetEvent(DynaPlex::RNG&) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State&, int64_t action) const;
			//You may also define this with a parameter DynaPlex::RNG&, for random initial states:
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			//Enables all MDPs to be constructer in a uniform manner. e
			explicit MDP(const DynaPlex::VarGroup&);
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
		};
	}
}

