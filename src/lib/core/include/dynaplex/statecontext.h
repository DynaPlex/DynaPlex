#pragma once
#include <cstdint>
#include <vector>
#include "dynaplex/statecategory.h"
#include "rng.h"

namespace DynaPlex {
	struct StateContext {
		//DynaPlex::StateCategory category;
		int64_t NextAction;
		std::vector<DynaPlex::RNG> RNGs;
		double CumulativeReturn;
		StateContext() = default;
	};
}