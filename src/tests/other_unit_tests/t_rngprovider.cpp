#include <gtest/gtest.h>
#include "dynaplex/rngprovider.h"
#include "dynaplex/error.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <vector>
#include <boost/math/special_functions/erf.hpp>

namespace DynaPlex::Tests {
	

	TEST(rngprovider, basics) {
		DynaPlex::RNGProvider provider{};
		DynaPlex::RNGProvider provider2{};

		provider.SeedEventStreams(false,123);
		provider2.SeedEventStreams(false,123);
		auto& rng = provider.GetEventRNG(1);
		auto& rng2 = provider2.GetEventRNG(1);
		return;

		std::vector<double> vec{};
		for (size_t i = 0; i < 3; i++)
		{//same seed results in same event stream. 
			double d = rng.genUniform();
			vec.push_back(d);
			ASSERT_EQ(d, rng2.genUniform());
		}
		//reset to initial value. 
		provider.SeedEventStreams(false,123);

		auto rng3 = provider.GetEventRNG(1);
		for (size_t i = 0; i < 3; i++)
		{
			double d = vec[i];
			ASSERT_EQ(d, rng3.genUniform());
		}

		DynaPlex::RNGProvider provider3{};

		//Not seeded. 
		ASSERT_THROW(provider3.GetInitiationRNG(), DynaPlex::Error);

		
		provider3.SeedEventStreams(false,123);

		{
			auto& rng3 = provider.GetInitiationRNG();
			auto& rng4 = provider2.GetInitiationRNG();
			for (size_t i = 0; i < 100; i++)
			{				
				ASSERT_EQ(rng3.genUniform(), rng4.genUniform());
			}
		}

	}

	// Helper function to calculate the ECDF
	std::vector<double> ecdf(const std::vector<double>& data) {
		std::vector<double> sorted_data = data;
		std::sort(sorted_data.begin(), sorted_data.end());
		std::vector<double> ecdf(data.size());

		for (size_t i = 0; i < data.size(); ++i) {
			ecdf[i] = (std::lower_bound(sorted_data.begin(), sorted_data.end(), data[i]) - sorted_data.begin()) / static_cast<double>(data.size());
		}
		return ecdf;
	}

	// Helper function for Kolmogorov-Smirnov Test critical value
	double ks_critical_value(double significance_level, size_t sample_size) {
		// This is an approximation. For exact values, use statistical tables.
		return std::sqrt(-0.5 * std::log(significance_level / 2)) / std::sqrt(sample_size);
	}

	bool isUniform(const std::vector<double>& data, double significance_level = 0.05) {
		auto data_ecdf = ecdf(data);
		double d_max = 0.0;
		for (size_t i = 0; i < data.size(); ++i) {
			double d = std::abs(data_ecdf[i] - data[i]);
			if (d > d_max) {
				d_max = d;
			}
		}
		double critical_value = ks_critical_value(significance_level, data.size());
		return d_max <= critical_value;
	}


	// Simplified Runs Test for Independence
	bool isIndependent(const std::vector<double>& data, double significance_level = 0.05, double threshold =0.5) {
		if (data.size() < 2) return true;

		int64_t runs = 1;
		int64_t n1 = 0;
		int64_t n2 = 0;


		for (size_t i = 1; i < data.size(); ++i) {
			if ((data[i] < threshold && data[i - 1] >= threshold) || (data[i] >= threshold && data[i - 1] < threshold)) {
				runs++;
			}
			if (data[i] < threshold) {
				n1++;
			}
			else {
				n2++;
			}
		}
	
		double expected_runs = 2.0 * n1 * n2 / (n1 + n2) + 1;
		double variance_runs = (2.0 * n1 * n2 * (2.0 * n1 * n2 - n1 - n2)) / (1.0* (n1 + n2) * (n1 + n2) * (n1 + n2 - 1));

	
		// Z-score
		double z = (runs - expected_runs) / std::sqrt(variance_runs);

		double z_critical = std::sqrt(2) * boost::math::erfc_inv(significance_level);
		return std::abs(z) < z_critical; 
	}


	TEST(rngprovider, iid_uniformity) {
		int64_t rng_seed = 123123;
		int number_of_samples = 1000;  // Number of different 'sample' values to test

		auto uniformity_failures = 0;
		auto independence_failures = 0;
		auto independence_failures_90 = 0;
		auto tries = 1000;
		auto significance = 0.1;
		for (size_t i = 0; i < tries; i++)
		{

			std::vector<double> first_drawn_numbers;
			first_drawn_numbers.reserve(number_of_samples);
			for (int sample = 0; sample < number_of_samples; ++sample) {
				DynaPlex::RNGProvider provider{};
				provider.SeedEventStreams(false, rng_seed, sample, i); // Only 'sample' is varied
				auto rng = provider.GetPolicyRNG();

				double first_number = rng.genUniform();
				first_drawn_numbers.push_back(first_number);
			}

			if (!isUniform(first_drawn_numbers,significance))
			{
				uniformity_failures++;
			}
			if (!isIndependent(first_drawn_numbers, significance,0.4))
			{
				independence_failures++;
			}
			if (!isIndependent(first_drawn_numbers, significance, 0.9))
			{
				independence_failures_90++;
			}
			
		}
		EXPECT_NEAR(significance * tries, uniformity_failures, 0.03 * tries);

		EXPECT_NEAR(significance * tries, independence_failures, 0.03 * tries);

		EXPECT_NEAR(significance * tries, independence_failures_90, 0.03 * tries);
	}

	TEST(rng, iid_uniformity) {
		int64_t rng_seed = 123123;
		auto sample = 123ll;

		int number_of_samples = 150;  // Number of different 'sample' values to test

		auto uniformity_failures = 0;
		auto independence_failures = 0;
		auto independence_failures_90 = 0;
		auto tries = 1000;
		auto significance = 0.1;
		for (size_t i = 0; i < tries; i++)
		{

			std::vector<double> first_drawn_numbers;
			first_drawn_numbers.reserve(number_of_samples);
			for (int stream = 0; stream < number_of_samples; ++stream) {
				auto rng = DynaPlex::RNG(false, rng_seed, sample, i,stream);

				double first_number = rng.genUniform();
				first_drawn_numbers.push_back(first_number);
			}

			if (!isUniform(first_drawn_numbers, significance))
			{
				uniformity_failures++;
			}
			if (!isIndependent(first_drawn_numbers, significance, 0.4))
			{
				independence_failures++;
			}
			if (!isIndependent(first_drawn_numbers, significance, 0.9))
			{
				independence_failures_90++;
			}

		}
		EXPECT_NEAR(significance * tries, uniformity_failures, 0.03 * tries);

		EXPECT_NEAR(significance * tries, independence_failures, 0.03 * tries);

		EXPECT_NEAR(significance * tries, independence_failures_90, 0.03 * tries);
	}


}