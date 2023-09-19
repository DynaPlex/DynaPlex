#include "dynaplex/rngprovider.h"
namespace DynaPlex {
	void RNGProvider::SeedEventStreams(const std::vector<uint32_t>& seed_base_list)
	{
		rng_vec.reserve(number_of_rngs);
		rng_vec.clear();

		auto list = seed_base_list;

		if (list.size() == 0)
		{
			throw DynaPlex::Error("RNGProvider::SeedEventStreams was called but a seed_base_list is empty");
		}

		while (rng_vec.size() < number_of_rngs)
		{
			//make sure the event_stream is unique for this number by appending it to the seed_list
			list.push_back(rng_vec.size());
			rng_vec.emplace_back(list);
			//remove appended element again. 
			list.pop_back();
		}
	}
}
