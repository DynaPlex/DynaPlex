#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace airplane /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		RuleBasedPolicy::RuleBasedPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
		}

		int64_t RuleBasedPolicy::GetAction(const MDP::State& state) const
		{
			if (state.RemainingSeats > 5)
			{
				return 1;//sell
			}
			if (state.PriceOfferedPerSeat > 1000.0)
			{//only sell to type 1 and 2 customers
				if (state.RemainingSeats <= 5 && state.RemainingSeats >= 1)
				{
					if (state.RemainingDays <= 9)
					{
						return 1;//sell
					}

				}
			}
			if (state.PriceOfferedPerSeat > 2000.0)
			{//only sell to type 1 customers
				if (state.RemainingSeats <= 5 && state.RemainingSeats >= 1)
				{
					if (state.RemainingDays >= 10)
					{
						return 1;//sell
					}

				}
			}
			return 0;//no sales
		}

	}
}