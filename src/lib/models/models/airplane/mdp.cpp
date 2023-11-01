#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"


namespace DynaPlex::Models {
	namespace airplane /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			//Needs to update later:
			vars.Add("valid_actions", 2);//We can either accept or reject each arriving customer.
			VarGroup feats{};
			vars.Add("features", feats);
			vars.Add("discount_factor", discount_factor);
			vars.Add("horizon_type", "finite");
			return vars;
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			//after processing this event, we await an action.
			state.cat = StateCategory::AwaitAction();
			state.NumSeatsRequested = event.NumSeats;
			state.PriceOfferedPerSeat = event.PriceOfferedPerSeat;
			return 0.0;
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			if (state.RemainingDays == 0)
			{
				//There should not be any sales in the last day! 
				std::cout << "something is wrong in airplane::ModifyStateWithAction" << std::endl;
				throw;
			}
			if (!IsAllowedAction(state, action))
			{
				throw DynaPlex::Error("airplane: action not allowed");
			};

			state.cat = StateCategory::AwaitEvent();//after processing this event, we await an event.

			if (action==0)
			{//reject offer
				state.RemainingDays--;//reduce the remaining days by 1
				return 0.0; //Note that flow ends when calling return, i.e.remainder of function is not carried out if action == 0
			}
			else
			{
				if (action == 1)
				{
					if (state.RemainingSeats < state.NumSeatsRequested)
					{
						//This action should not be allowed, something is wrong! 
						std::cout << "this action is not allowed in airplane::ModifyStateWithAction" << std::endl;
						throw;
					}
					//Subtract the requested seats from the remaining seats
					state.RemainingSeats -= state.NumSeatsRequested;
					//One day passes.
					state.RemainingDays--;
					//Note that DynaPlex is cost-based, so we return minus the reward here:
					double returnval = -state.PriceOfferedPerSeat * state.NumSeatsRequested;
					state.PriceOfferedPerSeat = 0.0;
					state.NumSeatsRequested = 0;
					return returnval;
				}
				else
				{
					//Note that NumValidActions is 2, so action can only be 0 or 1. 
					std::cout << "something is wrong in airplane::ModifyStateWithAction" << std::endl;
					throw;
				}
			}
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			//add all state variables
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("NumSeatsRequested", NumSeatsRequested);
			vars.Add("RemainingDays", RemainingDays);
			vars.Add("RemainingSeats", RemainingSeats);
			vars.Add("PriceOfferedPerSeat", PriceOfferedPerSeat);
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};			
			vars.Get("cat", state.cat);
			vars.Get("NumSeatsRequested", state.NumSeatsRequested);
			vars.Get("RemainingDays", state.RemainingDays);
			vars.Get("RemainingSeats", state.RemainingSeats);
			vars.Get("PriceOfferedPerSeat", state.PriceOfferedPerSeat);
			return state;
		}

		MDP::State MDP::GetInitialState() const
		{			
			State state{};
			state.cat = StateCategory::AwaitEvent();//or AwaitAction(), depending on logic
			//initiate other variables.
			state.NumSeatsRequested = 0;
			state.PriceOfferedPerSeat = 0.0;
			state.RemainingDays = InitialDays;
			state.RemainingSeats = InitialSeats;
			return state;
		}

		MDP::MDP(const VarGroup& config)
		{
			//In principle, state variables should be initiated as follows:
			//config.Get("name_of_variable",name_of_variable); 
			
			//we may also have config arguments that are not mandatory, and the internal value takes on 
			// a default value if not provided. Use sparingly. 
			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;

			config.Get("InitialDays", InitialDays);
			config.Get("RemainingSeats", InitialSeats);

			config.Get("PricePerSeatPerCustType", PricePerSeatPerCustType);
			config.Get("cust_dist", cust_dist);
			config.Get("seat_dist", seat_dist);
		
		}


		MDP::Event MDP::GetEvent(RNG& rng) const {
			int64_t custType = cust_dist.GetSample(rng);
			double pricePerSeat = PricePerSeatPerCustType[custType];
			//Note that the arrays are zero-based, so we need to add one here to get appropriate probabilities:
			size_t numSeats = seat_dist.GetSample(rng) + 1;
			return Event(numSeats, pricePerSeat);
		}

		

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			features.Add(state.RemainingDays);
			features.Add(state.NumSeatsRequested);
			features.Add(state.RemainingSeats);
			features.Add(state.PriceOfferedPerSeat);
		}
		

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			//custom policies, if added at some point, can be added here. For syntax, refer to models/models/lost_sales/mdp.cpp
			registry.Register<RuleBasedPolicy>("RuleBasedPolicy",
				"The heuristic rule as proposed by the manager");
		}
		
		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			if (action == 0)
			{//rejection is always allowed:
				return true;
			}
			//If we haven't returned, apparently action=1.
			//We can accept if the remaining seats is larger than or equal to the
			//requested seats:
			return state.NumSeatsRequested <= state.RemainingSeats;
		}


		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("airplane", "A relatively simple MDP for demonstrating the interface of the model", registry);
			//To use this MDP with dynaplex, register it like so, setting name equal to namespace and directory name
			// and adding appropriate description. 
			//DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
			//	"<id of mdp goes here, and should match namespace name and directory name>",
			//	"<description goes here>",
			//	registry); 
		}
	}
}

