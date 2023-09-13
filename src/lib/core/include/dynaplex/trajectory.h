#pragma once
#include <cstdint>
#include <vector>
#include "vargroup.h"
#include "statecategory.h"
#include "rngprovider.h"
#include "state.h"

namespace DynaPlex {
	struct Trajectory {


		VarGroup ToVarGroup() const;
		int64_t NextAction;
		StateCategory Category;
		double CumulativeReturn;
		DynaPlex::dp_State State;
		RNGProvider rngprovider;


		void Reset(const uint32_t& experiment_number, const uint32_t& world_rank, const bool& eval)
		{
			rngprovider.Reset(constructseeds(experiment_number, world_rank, eval));
		}

		Trajectory(int64_t NumEventRNGs) :
			NextAction{},
			Category{},
			CumulativeReturn{ 0.0 },
			rngprovider(NumEventRNGs),
			State{}
		{}
	private:
		std::vector<uint32_t> constructseeds(const uint32_t& experiment_number, const uint32_t& world_rank, const bool& eval);
		
	};
}