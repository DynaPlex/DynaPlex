	#pragma once
	#include <vector>
	#include "rng.h"
	#include "error.h"

	namespace DynaPlex {
		class RNGProvider {
		public:
			///returns the RNG stream for use in policies. 
			RNG& GetPolicyRNG()
			{
				if (rng_vec.empty())
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. Did you forget to Seed?");
				return rng_vec.at(0);
			}
			///returns the RNG stream to be used for getting initial states. 
			RNG& GetInitiationRNG()
			{
				if (rng_vec.empty())
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. Did you forget to Seed?");
				return rng_vec.at(1);
			}

			///Returns the rng associated with the a specific event/rng stream 0,1,etc.
			RNG& GetEventRNG(int64_t number)
			{			
				if (rng_vec.empty())
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. Did you forget to Seed?");
			
				if (number < 0 || number>1000)
					//Requirement of below 1000 should not be an issue for most designs, and having so many event streams
					//would be a strange design anyhow. The limit is not a hard limit, but going over the limit would require a redesign
					//of the seeding strategy. 
					throw DynaPlex::Error("RNGProvider: eventstream must be non-negative and below 1000");

				Expand(number + 3);
				return rng_vec.at(number+2);
			}

			RNGProvider() :rng_vec{}, global_seed{ 0 }, sample{ 0 }, trajectory{ 0 }, eval{ false }
			{}
			
			
			void SeedEventStreams(bool evaluation, int64_t rng_seed=13021985, int64_t sample = (1ll << 30)-1, int64_t trajectory = (1ll << 22 ) -1 );
			

		private:
			inline void Expand(int64_t size)
			{
				while (rng_vec.size() < size)
				{
					rng_vec.reserve(size);
					rng_vec.push_back(DynaPlex::RNG(eval, global_seed, sample, trajectory, rng_vec.size()));
				}
			}
			std::vector<DynaPlex::RNG> rng_vec;

			bool eval;
			int64_t global_seed, sample, trajectory;

		};
	}

