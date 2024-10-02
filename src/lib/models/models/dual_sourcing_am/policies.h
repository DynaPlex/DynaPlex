#pragma once
#include <cstdint>
#include "dynaplex/models/dual_sourcing/mdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace dual_sourcing /*must be consistent everywhere for complete mdp defininition and associated policies.*/
	{
		class DualIndexPolicy
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
			std::vector<int64_t> delta_;
			std::vector<int64_t> orderupto_am_;
		public:
			DualIndexPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t GetAction(const MDP::State& state) const;
		};

		class SingleIndexPolicy
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
			int64_t delta;
			int64_t orderupto_cm;
		public:
			SingleIndexPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t GetAction(const MDP::State& state) const;
		};

		class BaseStockPolicy
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;
			int64_t orderupto_cm;
		public:
			BaseStockPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t GetAction(const MDP::State& state) const;
		};
	}
}

