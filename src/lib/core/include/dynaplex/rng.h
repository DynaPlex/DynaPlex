#pragma once
#include <random>
#include <cstdint>
#include "pcg_random/pcg_random.hpp"
#include "dynaplex/error.h"
#include <vector>
#include <limits>
#include <stdexcept>
#include "xoshiro/xoshiro256plusplus.hpp"

namespace DynaPlex {

    class RNG {
    public:
        using type = XoshiroCpp::Xoshiro128Plus;

        explicit RNG(const std::initializer_list<std::uint32_t>& seed_data) {
            seed_generator(seed_data);
        }
        explicit RNG(const std::vector<std::uint32_t>& seed_data) {
            seed_generator(seed_data);
        }

        type& gen() {
            return generator_;
        }

        static uint32_t ToSeed(int64_t seed_candidate, std::string name) {
            constexpr int64_t max_uint32 = static_cast<int64_t>(std::numeric_limits<std::uint32_t>::max());

            if (seed_candidate > 0 && seed_candidate <= max_uint32) {
                return static_cast<uint32_t>(seed_candidate);
            }
            else {
                throw DynaPlex::Error(name + " : invalid rng_seed. Should be positive, non-zero number that fits in uint32_t");
            }
        }

        double genUniform() {
            return uniformDist(generator_);
        }

    private:
        type generator_;
        std::uniform_real_distribution<double> uniformDist{ 0.0, 1.0 };

        void seed_generator(const std::initializer_list<std::uint32_t>& seed_data) {
            std::array<std::uint32_t, 4> seed_array;
            std::seed_seq seq(seed_data.begin(), seed_data.end());
            seq.generate(seed_array.begin(), seed_array.end());
            generator_ = type(seed_array);
        }

        void seed_generator(const std::vector<std::uint32_t>& seed_data) {
            std::array<std::uint32_t, 4> seed_array;
            std::seed_seq seq(seed_data.begin(), seed_data.end());
            seq.generate(seed_array.begin(), seed_array.end());
            generator_ = type(seed_array);
        }
        
    };

    //class RNG {
    //public:
    //    using type = pcg_cpp::pcg64;
    //    explicit RNG(const std::initializer_list<std::uint32_t>& seed_data) {
    //        seed_generator(seed_data);
    //    }
    //    explicit RNG(const std::vector<std::uint32_t>& seed_data) {
    //        seed_generator(seed_data);
    //    }

    //    type& gen() {
    //        return generator_;
    //    }

    //    static uint32_t ToSeed(int64_t seed_candidate, std::string name)
    //    {
    //        constexpr int64_t max_uint32 = static_cast<int64_t>(std::numeric_limits<std::uint32_t>::max());

    //        if (seed_candidate > 0 && seed_candidate <= max_uint32) {
    //            return static_cast<uint32_t>(seed_candidate);
    //        }
    //        else
    //            throw DynaPlex::Error(name + " : invalid rng_seed. Should be positive, non-zero number that fits in uint32_t");
    //    }

    //    double genUniform() {
    //        return uniformDist(generator_);
    //    }

    //private:
    //    type generator_;

    //    std::uniform_real_distribution<double> uniformDist{ 0.0, 1.0 };

    //    // Utility function to create a seed_seq from seed data and seed the generator
    //    void seed_generator(const std::initializer_list<std::uint32_t>& seed_data) {


    //        //auto seq = pcg_extras::seed_seq_from<type>(seed_data.begin(), seed_data.end());
    //        std::seed_seq seq(seed_data.begin(), seed_data.end());
    //        generator_.seed(seq);
    //    }
    //    // Utility function to create a seed_seq from seed data and seed the generator
    //    void seed_generator(const std::vector<std::uint32_t>& seed_data) {
    //        auto seq = std::seed_seq(seed_data.begin(), seed_data.end());
    //        generator_.seed(seq);// type(pcg_extras::seed_seq_from<type>(seed_data.begin(), seed_data.end()));
    //    }
    //};

}  // namespace DynaPlex
