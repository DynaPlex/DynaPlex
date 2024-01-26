#include "dynaplex/trajectory.h"
namespace DynaPlex {
	
	Trajectory::Trajectory(int64_t externalIndex):
	    NextAction{},
		Category{},
		PeriodCount{ 0 },
		EffectiveDiscountFactor{ 1.0 },
		CumulativeReturn{ 0.0 },
		state{},
		RNGProvider(),
		ExternalIndex{ externalIndex }
	{}

	void Trajectory::Reset()
	{
		CumulativeReturn = 0.0;
		EffectiveDiscountFactor = 1.0;
		PeriodCount = 0;
	}

	void Trajectory::Reset(DynaPlex::dp_State&& State)
	{
		state = std::move(State);
		Reset();
	}


	
}
