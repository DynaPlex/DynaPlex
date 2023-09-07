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
		std::vector<DynaPlex::RNG> RNGs;
		double CumulativeReturn;

		DynaPlex::dp_State State;

		Trajectory() = default;
	};
}