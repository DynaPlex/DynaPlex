#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace empty_example /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		EmptyPolicy::EmptyPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			//Here, you may initiate any policy parameters.
		}

		int64_t EmptyPolicy::GetAction(const MDP::State& state) const
		{
			//Implement custom policy, and remove below line.
			throw DynaPlex::NotImplementedError();
		}
	}
}