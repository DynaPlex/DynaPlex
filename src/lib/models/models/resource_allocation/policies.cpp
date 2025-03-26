#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace resource_allocation
	{

		ShortestProcessingTime::ShortestProcessingTime(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{			
		}

		int64_t ShortestProcessingTime::GetAction(const MDP::State& state) const
		{
			int64_t action = mdp->actions.size();
			double lowest_expectation = std::numeric_limits<double>::infinity();
			for (int64_t i = 0; i < mdp->actions.size(); i++)
			{
				if (mdp->IsAllowedAction(state,i))
				{
					auto expectation = mdp->actions.at(i).expected_duration;
					if (expectation < lowest_expectation)
					{
						lowest_expectation = expectation;
						action = i;
					}
				}
			}
			return action;
		}
	}
}