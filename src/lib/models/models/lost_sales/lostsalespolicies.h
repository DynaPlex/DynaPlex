#pragma once
#include <cstdint>
#include "lostsalesmdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace lost_sales /*must be consistent everywhere for complete mdp defininition and associated policies.*/
	{
		// Forward declaration
		class MDP;

		class BaseStockPolicy
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
		public:
			BaseStockPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& varGroup);
			int64_t GetAction(const MDP::State& state) const;
		};

	}
}

