#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"


namespace DynaPlex::Models {
	using namespace DynaPlex;
	namespace empty_example /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			//Needs to update later:
			vars.Add("valid_actions", 1);
			//VarGroup feats{};
			//vars.Add("features", feats);
			vars.Add("discount_factor", discount_factor);
			return vars;
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			throw DynaPlex::NotImplementedError();
			//implement change to state
			// do not forget to update state.cat. 
			//returns costs.
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			throw DynaPlex::NotImplementedError();
			//implement change to state. 
			// do not forget to update state.cat. 
			//returns costs. 
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			//add any other state variables. 
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};			
			vars.Get("cat", state.cat);
			//initiate any other state variables. 
			return state;
		}

		MDP::State MDP::GetInitialState() const
		{			
			State state{};
			state.cat = StateCategory::AwaitEvent();//or AwaitAction(), depending on logic
			//initiate other variables. 
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
		
		}


		MDP::Event MDP::GetEvent(RNG& rng) const {
			throw DynaPlex::NotImplementedError();
		}

		

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			throw DynaPlex::NotImplementedError();
		}
		

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			//custom policies, if added at some point, can be added here. For syntax, refer to models/models/lost_sales/mdp.cpp
		}
		
		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			throw DynaPlex::NotImplementedError();
		}


		void Register(DynaPlex::Registry& registry)
		{
			//To use this MDP with dynaplex, register it like so, setting name equal to namespace and directory name
			// and adding appropriate description. 
			//DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
			//	"<id of mdp goes here, and should match namespace name and directory name>",
			//	"<description goes here>",
			//	registry); 
		}
	}
}

