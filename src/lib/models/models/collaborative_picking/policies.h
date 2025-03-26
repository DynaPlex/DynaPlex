#pragma once
#include <cstdint>
#include "dynaplex/models/collaborative_picking/mdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace collaborative_picking
	{
		class ClosestPair
		{
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
		public:
			ClosestPair(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t GetAction(const MDP::State& state) const;
		};

	}
}

