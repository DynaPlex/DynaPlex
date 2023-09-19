	#pragma once
	#include <initializer_list>
	#include "rng.h"
	#include "error.h"

	namespace DynaPlex {
		class RNGProvider {
		public:
			///returns the RNG stream for use in policies. 
			RNG& GetPolicyRNG()
			{
				if (rng_vec.empty())
				{
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. Did you forget to Seed?");
				}
				return rng_vec.back();
			}
			///returns the RNG stream to be used for getting initial states. 
			RNG& GetInitiationRNG()
			{
				if (rng_vec.empty())
				{
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. Did you forget to Seed?");
				}
				return rng_vec.back();
			}

			///Returns the rng associated with the a specific event/rng stream 0,1,etc.
			RNG& GetEventRNG(uint32_t number)
			{			
				if (number >= rng_vec.size())
				{ //note that last element is reserved for initial states and policy
					std::string message = "RNGProvider: No rng available for event number " + std::to_string(number);
					if (number == 0)
						message += "\nDid you forget to Seed?";
					throw DynaPlex::Error(message);
				}
				return rng_vec[number];
			}

			RNGProvider() :
				RNGProvider(1)
			{}

			
			RNGProvider(int64_t NumEventRNGs) :
				number_of_rngs{ NumEventRNGs + 1 } 
			{
				if (NumEventRNGs < 0)
				{
					throw DynaPlex::Error("RNGProvider: NumEventRNGs cannot be negative");
				}
			}
			
			
			/** 
			 *Resets the event streams using some seed_base_list
			 */
			void SeedEventStreams(const std::vector<uint32_t>& seed_base_list);
			

		private:
			int64_t number_of_rngs; 
			std::vector<DynaPlex::RNG> rng_vec;
			std::vector<uint32_t> list;

		};
	}

