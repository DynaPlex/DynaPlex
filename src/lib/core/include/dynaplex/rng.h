#pragma once
#include "dynaplex/error.h"
#include "xoshiro/xoshiro256plusplus.hpp"

namespace DynaPlex {

	class RNG {
	public:
		using type = XoshiroCpp::Xoshiro256PlusPlus;
		/**
		 * Initiates the random number generator, based on provided parameters: Eval, global_seed, sample, trajectory, stream.
		 *
		 * This function combines the values of sample, trajectory, and stream to generate a unique seed,
		 * and then XORs this combined value with the global_seed. The generated seed is used for generating an RNG for training purposes,
		 * ensuring variability and reproducibility based on the input parameters.
		 *
		 * Parameters:
		 * - eval (bool): Indicates whether the rng will be used for training or evaluation purposes. Seeds used for training purposes are non-overlapping with
		 * those used for evaluation purposes. 
		 * - global_seed (int64_t): A global seed value used to introduce additional variability. Must be a non-negative value
		 *   (most significant bit must be 0).	
		 * - sample (int64_t): A value representing the sample. Must be non-negative and less than 2^30.
		 * - trajectory (int64_t): A value representing the trajectory. Must be non-negative and less than 2^23.
		 * - stream (int64_t): A value representing the stream. Must be non-negative and less than 2^10.
			 *
		 * Throws if any of these requirements are not met.
		 *
		 * The function first combines the sample, trajectory, and stream values by left-shifting and OR-ing them, ensuring that these values
		 * are placed in separate bit ranges of a 64-bit integer. Finally, the function XORs the combined value with
		 * the global_seed to produce the final training seed, which is used to seed the generator.
		 *
		 * Return:
		 * - DynaPlex::RNG based on the generated training seed.
		 */
	    RNG(bool eval, int64_t global_seed = 15112017, int64_t sample = (1LL << 30) - 1, int64_t trajectory = (1LL << 23) - 1, int64_t stream = (1LL << 10) - 1);

		
		type& gen() {
			return generator_;
		}

		int64_t genInt() {
			return static_cast<int64_t>(generator_());
		}

		double genUniform() {
			return XoshiroCpp::DoubleFromBits(generator_());
		}

	private:
		type generator_;
		RNG(uint64_t seed);
	};

}  // namespace DynaPlex
