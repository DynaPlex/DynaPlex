#include "include/dynaplex/rng.h"
namespace DynaPlex {

    inline uint64_t CombineSeeds(bool eval, int64_t global_seed, int64_t sample, int64_t trajectory, int64_t stream)
    {
        // Check ranges
        if (sample < 0 || sample >= (1LL << 30)) {
            throw DynaPlex::Error("get_training_seed::Sample is out of range");
        }
        if (trajectory < 0 || trajectory >= (1LL << 23)) {
            throw DynaPlex::Error("get_training_seed::Trajectory is out of range");
        }
        if (stream < 0 || stream >= (1LL << 10)) {
            throw DynaPlex::Error("get_training_seed::Stream is out of range");
        }

        // Check global_seed's most significant bit
        if (global_seed < 0) {
            throw DynaPlex::Error("get_training_seed::Global seed's first bit must be zero (i.e. global seed must be non-negative)");
        }

        // Combining the values
        uint64_t seed = (static_cast<uint64_t>(sample) << (23 + 10)) |
                           (static_cast<uint64_t>(trajectory) << 10) |
                            static_cast<uint64_t>(stream);

        // XOR with global_seed
        seed ^= static_cast<uint64_t>(global_seed);
        uint64_t first_bit_mask = static_cast<uint64_t>(1) << 63;
        if (seed & (first_bit_mask)) {
            throw DynaPlex::Error("Logic error: MSB is non-zero after XOR operation");
        }
        // Set the MSB if eval is true
        if (eval) {
            seed |= first_bit_mask; // Set the MSB
        }

        return seed;
    }

    RNG::RNG(uint64_t seed): generator_(seed)
    {

    }

    RNG::RNG(bool eval, int64_t global_seed, int64_t sample, int64_t trajectory, int64_t stream ):
        generator_(CombineSeeds(eval, global_seed, sample, trajectory, stream))
    {
    }

}