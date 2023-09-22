#include "dynaplex/trajectory.h"
namespace DynaPlex {
	
	Trajectory::Trajectory(int64_t NumEventRNGs, int64_t externalIndex):
	    NextAction{},
		Category{},
		EventCount{ 0 },
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
		EventCount = 0;
	}

	void Trajectory::Reset(DynaPlex::dp_State&& State)
	{
		state = std::move(State);
		Reset();
	}

	void Trajectory::SeedRNGProvider(const DynaPlex::System& system, bool eval, int64_t experiment_number, uint32_t thread_number)
	{
		std::vector<uint32_t> seeds{};
		seeds.reserve(5);

		uint64_t uniqueThreadId = static_cast<uint64_t>(thread_number) * system.WorldSize() + system.WorldRank();

		if (uniqueThreadId > std::numeric_limits<uint32_t>::max())
		{
			throw DynaPlex::Error("Trajectory: Unique thread ID exceeds uint32_t maximum value.");
		}

		seeds.push_back(static_cast<uint32_t>(uniqueThreadId));

		//To avoid any overlap between trajectories used for training and
		//those used for evaluation. 
		if (eval)
		{
			seeds.push_back(26071983);
		}
		else
		{
			seeds.push_back(11112014);
		}
		uint32_t high_bits = static_cast<uint32_t>(experiment_number >> 32);
		uint32_t low_bits = static_cast<uint32_t>(experiment_number & 0xFFFFFFFF);

		seeds.push_back(high_bits);
		seeds.push_back(low_bits);
		//this needs to be adapted, since experiment_number is int64_t. Can we somehow split it?
		seeds.push_back(experiment_number);

		RNGProvider.SeedEventStreams(seeds);
	}
	
}
