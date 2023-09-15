#pragma once
#include <cstdint>
#include "lostsalesmdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace LostSales /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{
		// Forward declarations
		class MDP;

		//class MDP {
		//public:
		//	class State;
		//};

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

