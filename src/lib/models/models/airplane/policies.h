#pragma once
#include <cstdint>
#include "mdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace airplane /*must be consistent everywhere for complete mdp defininition and associated policies.*/
	{
		// Forward declaration
		class MDP;

		class RuleBasedPolicy
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
		public:
			RuleBasedPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t GetAction(const MDP::State& state) const;
		};

	}
}

