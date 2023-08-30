#pragma once
#include <initializer_list>
#include "pcg_random/pcg_random.hpp"
#include <random>
namespace DynaPlex {

    class RNG {
    public:
        using type = pcg_cpp::pcg64;       
        explicit RNG(std::initializer_list<std::uint32_t> seed_data) {
            seed_generator(seed_data);
        }

        type& gen() {
            return generator_;
        }

        double genUniform() {
            return uniformDist(generator_);
        }

    private:
        type generator_;

        std::uniform_real_distribution<double> uniformDist{ 0.0, 1.0 };

        // Utility function to create a seed_seq from seed data and seed the generator
        void seed_generator(std::initializer_list<std::uint32_t> seed_data) {
            std::seed_seq seq(seed_data.begin(), seed_data.end());
            generator_.seed(seq);
        }
    };

}  // namespace DynaPlex
