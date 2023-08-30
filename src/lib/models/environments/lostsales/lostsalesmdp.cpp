#include "lostsalesmdp.h"
#include "dynaplex/mdpregistrar.h"

namespace DynaPlex::Models {
	namespace LostSales /*keep this in line with id below and with namespace name in header*/
	{
		DynaPlex::VarGroup MDP::GetStaticInfo() const
		{
			DynaPlex::VarGroup vars;		

			vars.Add("num_valid_actions", MaxOrderSize + 1);


			//To enable clients to figure out how this mdp was configured.  
			vars.Add("mdp_config", varGroup);
			//potentially add any stuff that was computed for diagnostics purposes
			//not used by dynaplex framework itself. 
			DynaPlex::VarGroup diagnostics{};			
			diagnostics.Add("MaxOrderSize", MaxOrderSize);
			diagnostics.Add("MaxSystemInv", MaxSystemInv);
			vars.Add("diagnostics", diagnostics);
			
			return vars;
		}

		double MDP::ModifyStateWithAction(State& state, int64_t action) const
		{
			state.state += action;
			return 0.0;
		}


		MDP::State MDP::GetInitialState() const
		{
			return State{ 123 };
		}

		MDP::MDP(const DynaPlex::VarGroup& varGroup) :
			varGroup{ varGroup }
		{
			varGroup.Get("p", p);
			varGroup.Get("h", h);
			varGroup.Get("leadtime", leadtime);
			varGroup.Get("demand_dist",demand_dist);

		}


		MDP::Event MDP::GetEvent(DynaPlex::RNG& rng) const {
			return demand_dist.GetSample(rng);
		}

		double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const
		{
			return 0.0;
		}


		bool MDP::AwaitsAction(const State& state) const {
			return false;
		}
		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			return false;
		}
	}

	//static registrar (only in .cpp file): includes MDP in the central registry, such that DynaPlex::GetMDP can locate it 
	static MDPRegistrar<LostSales::MDP> registrar(/*id*/"lost_sales",/*optional brief description*/ "Canonical lost sales problem.");
}

