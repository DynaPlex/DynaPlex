#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace perishable_systems /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{
		//MDP and State refer to the specific ones defined in current namespace
		BaseStockPolicy::BaseStockPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			config.GetOrDefault("base_stock_level", base_stock_level, mdp->MaxSystemInv);
		}

		int64_t BaseStockPolicy::GetAction(const MDP::State& state) const
		{
			if (base_stock_level > state.state_vector.back()) {
				return base_stock_level - state.state_vector.back();
			}
			else {
				return 0;
			}
		}
	}
}