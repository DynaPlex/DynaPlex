#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"


namespace DynaPlex::Models {
	namespace bin_packing /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			vars.Add("valid_actions", number_of_bins);

			vars.Add("discount_factor", discount_factor);

			//This indicates that the MDP never terminates. 
			//It may be used by various algorithms. 
			//infinite is default, other value is finite for algorithms that are guaranteed to reach a final state at some point. 
			vars.Add("horizon_type", "infinite");



			return vars;
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			state.cat = StateCategory::AwaitAction();
			state.upcoming_weight = event;
			return 0.0;
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			state.cat = StateCategory::AwaitEvent();
			state.weight_vector[action] += state.upcoming_weight;
			int64_t diff = state.weight_vector[action] - max_bin_size;

			if (diff >= 0)
			{
				state.weight_vector[action] = 0;
				return diff;
			}
			return 0.0;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("weight_vector", weight_vector);
			vars.Add("upcoming_weight", upcoming_weight);
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};			
			vars.Get("cat", state.cat);
			vars.Get("weight_vector", state.weight_vector);
			vars.Get("upcoming_weight", state.upcoming_weight);		
			return state;
		}

		MDP::State MDP::GetInitialState() const
		{			
			State state{};
			state.cat = StateCategory::AwaitEvent();
			state.weight_vector = std::vector<int64_t>(number_of_bins,0);
			state.upcoming_weight = 0;
			return state;
		}

		MDP::MDP(const VarGroup& config)
		{		
			config.Get("max_bin_size", max_bin_size);
			config.Get("number_of_bins", number_of_bins);
			config.Get("weight_dist", weight_dist);
			//to do: check weight_dist validity
			
			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;
		}


		MDP::Event MDP::GetEvent(DynaPlex::RNG& rng) const {
			return weight_dist.GetSample(rng); 
		}

		std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities() const {
			return weight_dist.QuantityProbabilities();
		}


		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			features.Add(state.weight_vector);
			features.Add(state.upcoming_weight);
		}
		

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			//no custom policies registered currently for this MDP. 
		}
		
		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			return true;
		}

		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
				/*=id though which the MDP will be retrievable*/ "bin_packing",
				/*description*/ "a dynamic bin-packing problem. Weight are revealed one by one, and must be added to on of several bins. When a bin exceeds maximum_weight, it is dispatched and hence emptied. Any weight exceeding the maximum_weight is cost. I.e. we must avoid making the bins fuller than strictly neccesary for dispatch. ",
				registry); 
		}
	}
}

