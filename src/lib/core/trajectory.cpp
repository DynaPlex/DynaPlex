#include "dynaplex/trajectory.h"
namespace DynaPlex {
	
	Trajectory::Trajectory(int64_t NumEventRNGs, int64_t externalIndex):
	    NextAction{},
		Category{},
		PeriodCount{ 0 },
		EffectiveDiscountFactor{ 1.0 },
		CumulativeReturn{ 0.0 },
		state{},
		RNGProvider(NumEventRNGs),
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

	void Trajectory::SeedRNGProvider(bool eval, int64_t experiment_id, uint32_t secondary_id)
	{
		std::vector<uint32_t> seeds{};
		seeds.reserve(3);		
		uint32_t high_bits = static_cast<uint32_t>(experiment_id >> 32);
		uint32_t low_bits = static_cast<uint32_t>(experiment_id & 0xFFFFFFFF);
		seeds.push_back(low_bits);
		seeds.push_back(high_bits);
		//if (high_bits)
		//{
		////	throw DynaPlex::Error("too high value used for seed");
		//	seeds.push_back(high_bits);
		//}

		//seeds.push_back(experiment_id);
	
		uint64_t eval_int = (eval ? 1 : 0);	
		uint64_t combined_id = secondary_id *2+eval_int;
		if (combined_id > std::numeric_limits<uint32_t>::max())
		{
			throw DynaPlex::Error("Trajectory: combined_id ID exceeds uint32_t maximum value. Avoid using too large values for secondary id. ");
		}
		seeds.push_back(static_cast<uint32_t>(combined_id));

		

		RNGProvider.SeedEventStreams(seeds);
	}
	
}
