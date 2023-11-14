#pragma once
#include <cstdint>
#include "mdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace order_picking /*must be consistent everywhere for complete mdp defininition and associated policies.*/
	{
		// Forward declaration
		class MDP;

		class GreedyHeuristic
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
			
			bool coordinated;
			bool costBased;
			std::vector<double> costs;


		public:
			GreedyHeuristic(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t FindAction(const MDP::State& state) const;
			int64_t GetAction(const MDP::State& state) const;
		};

	}
}