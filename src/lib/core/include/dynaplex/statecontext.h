#pragma once
#include <cstdint>
#include <vector>
#include "rng.h"

namespace DynaPlex {
	struct StateContext {
		int64_t NextAction;
		std::vector<DynaPlex::RNG> RNGs;
		double CumulativeReturn;
		StateContext() = default;
	};
}