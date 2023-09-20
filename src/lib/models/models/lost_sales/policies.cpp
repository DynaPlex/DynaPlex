#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace lost_sales /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		BaseStockPolicy::BaseStockPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& varGroup)
			:mdp{ mdp }
		{
			//Any initiation goes here. 
		}

		int64_t BaseStockPolicy::GetAction(const MDP::State& state) const
		{
			int64_t action = mdp->MaxSystemInv - state.total_inv;
			//We maximize, so actually this is capped base-stock. 
			if (action > mdp->MaxOrderSize)
			{
				action = mdp->MaxOrderSize;
			}
			return action;
		}

	}
}