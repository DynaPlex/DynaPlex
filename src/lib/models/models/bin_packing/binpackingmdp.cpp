#include "binpackingmdp.h"
#include "dynaplex/erasure/mdpregistrar.h"


namespace DynaPlex::Models {
	using namespace DynaPlex;
	namespace bin_packing /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;		
			vars.Add("valid_actions", number_of_bins);

			VarGroup feats{};
			vars.Add("features", feats);
			vars.Add("discount_factor", discount_factor);
			return vars;
		}

		double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const
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


		MDP::State MDP::GetInitialState() const
		{			
			State state{};
			state.cat = StateCategory::AwaitEvent();
			state.weight_vector = std::vector<int64_t>(number_of_bins,0);
			state.upcoming_weight = 0;
			return state;
		}

		MDP::MDP(const VarGroup& varGroup) :
			varGroup{ varGroup }
		{

		
			varGroup.Get("max_bin_size", max_bin_size);
			varGroup.Get("number_of_bins", number_of_bins);
			varGroup.Get("weight_dist", weight_dist);
			
		    //check for possible negative values of weight_dist. 

			//providing discount_factor is optional. 
			if (varGroup.HasKey("discount_factor"))
				varGroup.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;
		}


		MDP::Event MDP::GetEvent(RNG& rng) const {
			return weight_dist.GetSample(rng);
		}

		

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			throw DynaPlex::Error("get features not implemented on lost_sales");
		}
		

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{//Here, we register any custom heuristics we want to provide for this MDP.	
		 //On the generic DynaPlex::MDP constructed from this, these heuristics can be obtained
		 //in generic form using mdp->GetPolicy(VarGroup vars), with the id in var set
		 //to the corresponding id given below.
		//	registry.Register<BaseStockPolicy>(/*=id though which the policy will be retrievable*/"base_stock",
		////	/*description*/	"Base-stock policy with fixed, non-adjustable base-stock level equal"
		//		" to the bound on system inventory discussed in Zipkin (2008)");
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

