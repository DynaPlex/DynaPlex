#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"


namespace DynaPlex::Models {
	namespace airplane /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			//We can either accept or reject each arriving customer:
			vars.Add("valid_actions", 2);
			vars.Add("horizon_type", "finite");
			return vars;
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			//after processing this event, we await an action.
			state.cat = StateCategory::AwaitAction();
			state.PriceOfferedPerSeat = event.PriceOfferedPerSeat;

			if (state.RemainingDays==0 || state.RemainingSeats == 0)
			{//here, we check if the MDP should terminate
				state.cat = StateCategory::Final();
			}
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

			state.cat = StateCategory::AwaitEvent();//after processing this action, we await an event.

			if (action==0)
			{//reject offer
				state.RemainingDays--;//reduce the remaining days by 1
				return 0.0; //Note that flow ends when calling return, i.e.remainder of function is not carried out if action == 0
			}
			else
			{
				if (action == 1)
				{
					//Subtract the requested seats from the remaining seats
					state.RemainingSeats--;
					if(state.RemainingSeats<0)
						throw DynaPlex::Error("airplane:: Sold too many seats.");
					//One day passes.
					state.RemainingDays--;
					//Note that DynaPlex is by default cost-based, so we return negative reward here:
					double returnval = -state.PriceOfferedPerSeat;
					state.PriceOfferedPerSeat = 0.0;
					return returnval;
				}
				else
				{
					throw DynaPlex::Error("airplane:: Invalid action chosen.");
				}
			}
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			//add all state variables
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("RemainingDays", RemainingDays);
			vars.Add("RemainingSeats", RemainingSeats);
			vars.Add("PriceOfferedPerSeat", PriceOfferedPerSeat);
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};			
			vars.Get("cat", state.cat);
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
			state.PriceOfferedPerSeat = 0.0;
			state.RemainingDays = InitialDays;
			state.RemainingSeats = InitialSeats;
			return state;
		}

		MDP::MDP(const VarGroup& config)
		{

			config.Get("InitialDays", InitialDays);
			config.Get("InitialSeats", InitialSeats);

			
			PricePerSeatPerCustType = { 3000.0, 2000.0, 1000.0 };
			//probability distribution 0 (w. prob 0.4), 1 (w. prob 0.3), 2 (w. prob 0.3). 
			cust_dist = DiscreteDist::GetCustomDist({ 0.4,0.3,0.3 });

			//Of course, any MDP property can be parameterized, but you can also
			//fix some things - configuration can always be expanded later.
			/*
			if(config.HasKey("PricePerSeatPerCustType"))
				config.Get("PricePerSeatPerCustType", PricePerSeatPerCustType);
			if (config.HasKey("cust_dist"))
				config.Get("cust_dist", cust_dist);
	
			//extra checks of input values (optional):
			if (cust_dist.Min() != 0)
				throw DynaPlex::Error("Airline MDP: initialization error.");
			if(PricePerSeatPerCustType.size()!=cust_dist.Max()+1)
				throw DynaPlex::Error("Airline MDP: initialization error.");
			*/
		}


		MDP::Event MDP::GetEvent(RNG& rng) const {
			//generate an event using the custom discrete distribution (see mdp_config_0.json)
			int64_t custType = cust_dist.GetSample(rng);
			double pricePerSeat = PricePerSeatPerCustType.at(custType);
			return Event(pricePerSeat);
		}


		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			//state features as supplied to learning algorithms:
			features.Add(state.RemainingDays);
			features.Add(state.RemainingSeats);
			features.Add(state.PriceOfferedPerSeat);
		}
		

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<RuleBasedPolicy>("rule_based",
				"The heuristic rule as proposed by the manager");
		}
		
		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			if (action == 0)
			{//rejection is always allowed:
				return true;
			}
			//If we haven't returned, apparently action=1.
			throw DynaPlex::Error("please check below line");
			//Selling a seat is allowed if there is at least one seat left:
			//return state.RemainingSeats > 0;
			//this is wrong?
			return 0 <= state.RemainingSeats;
		}


		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("airplane", "A relatively simple MDP for demonstrating the interface of the model", registry);
		}
	}
}