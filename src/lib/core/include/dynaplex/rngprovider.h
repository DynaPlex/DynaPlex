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
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. ");
				}
				return rng_vec.back();
			}
			///returns the RNG stream to be used for getting initial states. 
			RNG& GetInitiationRNG()
			{
				if (rng_vec.empty())
				{
					throw DynaPlex::Error("RNGProvider: Attempt to get RNG from empty provider. ");
				}
				return rng_vec.back();
			}

			///Returns the rng associated with the a specific event/rng stream 0,1,etc.
			RNG& GetEventRNG(uint32_t number)
			{			
				if (number >= rng_vec.size())
				{ //note that last element is reserved for initial states and policy
					throw DynaPlex::Error("RNGProvider: No rng available for this event number");
				}
				return rng_vec[number];
			}

			RNGProvider() :
				RNGProvider(1)
			{}

			
			RNGProvider(int64_t NumEventRNGs,const std::vector<uint32_t>& seed_base_list = std::vector<uint32_t>{}) :
				number_of_rngs{ NumEventRNGs + 1 },	list {
				seed_base_list
			}, rng_vec{}
			{				
				if (NumEventRNGs < 0)
				{
					throw DynaPlex::Error("RNGProvider: NumEventRNGs cannot be negative");
				}
				if (list.size() > 0)
				{
					Reset();
				}
			}
			RNGProvider(int64_t NumEventRNGs, std::vector<uint32_t>&& seed_base_list):
				number_of_rngs{ NumEventRNGs + 1 }, list{ std::move(seed_base_list) }, rng_vec{}
			{	
				if (NumEventRNGs < 0)
				{
					throw DynaPlex::Error("RNGProvider: NumEventRNGs cannot be negative");
				}
				if (list.size() > 0)
				{
					Reset();
				}
			}

			void Reset(std::vector<uint32_t>&& seed_base_list)
			{
				if (seed_base_list.size() > 0)
				{
					list = std::move(seed_base_list);
				}
				Reset();
			}

			/** Resets the event streams to some initial state
			  * if provided a seed_base_list, this will be used. 
			  * if not provided, the previously provided seed_base_list will be used. 
			  */
			void Reset(const std::vector<uint32_t>& seed_base_list = std::vector<uint32_t>{})
			{
				rng_vec.reserve(number_of_rngs);
				rng_vec.clear();
				if (seed_base_list.size() > 0)
				{
					list = seed_base_list;
				}
				if (list.size() == 0)
				{
					throw DynaPlex::Error("RNGProvider::Reset was called but a seed_base_list was never provided");
				}			

				while (rng_vec.size() <number_of_rngs)
				{
					//make sure the event_stream is unique for this number by appending it to the seed_list
					list.push_back(rng_vec.size());
					rng_vec.emplace_back(list);
					//remove appended element again. 
					list.pop_back();
				}
			}

		private:
			int64_t number_of_rngs; 
			std::vector<DynaPlex::RNG> rng_vec;
			std::vector<uint32_t> list;

		};
	}

