#pragma once
#include <cstdint>
#include <vector>
#include "dynaplex/statecategory.h"
#include "rng.h"
#include "dynaplex/state.h"
#include "dynaplex/vargroup.h"

namespace DynaPlex {
	struct Trajectory {
		VarGroup ToVarGroup() const;
		int64_t NextAction;
		StateCategory Category;
		std::vector<DynaPlex::RNG> RNGs;
		double CumulativeReturn;

		DynaPlex::dp_State State;

		Trajectory()
			:RNGs{},
			Category{},
			CumulativeReturn{ 0.0 }
		{
			RNGs.reserve(2);
			RNGs.push_back(DynaPlex::RNG{ 0 });
			RNGs.push_back(DynaPlex::RNG{ 1 });

		}
	};
}