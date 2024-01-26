#include "dynaplex/rngprovider.h"
namespace DynaPlex {
	void RNGProvider::SeedEventStreams(bool evaluation, int64_t global_seed, int64_t sample, int64_t trajectory)
	{
		this->global_seed = global_seed;
		this->sample = sample;
		this->trajectory = trajectory;
		this->eval = evaluation;

		rng_vec.clear();
		//start with 3 event streams.
		Expand(3);

	}
}
