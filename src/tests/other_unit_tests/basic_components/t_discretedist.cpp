#include <iostream>
#include <deque>
#include <gtest/gtest.h>
#include <vector>
#include <iterator>
#include "dynaplex/error.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/rng.h"
#include "dynaplex/policycomparison.h"

namespace DynaPlex::Tests {

	using namespace DynaPlex;

	std::map<int64_t, size_t> GatherBucketedSampleMeans(DiscreteDist& dist, size_t numSamples, size_t numMeans) {
		DynaPlex::RNG rng{ 26071983 ,1234}; // Using a fixed seed for reproducibility

		double trueMean = dist.Expectation();
		double trueVariance = dist.Variance();
		double sdOfSampleMean = std::sqrt(trueVariance / numSamples);

		std::map<int64_t, size_t> buckets; // Map from bucket index to count of means

		for (size_t meanIndex = 0; meanIndex < numMeans; ++meanIndex) {
			std::vector<double> vec;
			vec.reserve(numSamples);

			for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
				vec.push_back(dist.GetSample(rng));
			}
				
			auto comparison = PolicyComparison::GetComparison(vec);

			double deviationFromTrueMean = comparison.mean(0) - trueMean;

			// Assigning the deviation to a bucket using the standard deviation of the sample mean
			int64_t bucketIndex = static_cast<int64_t>(std::floor(deviationFromTrueMean / sdOfSampleMean));
			buckets[bucketIndex]++;
		}

		return buckets;
	}


	TEST(discretedist, GetSample) {


		int numMeans = 500;
		int numSamples = 500;
		{
			std::vector<double> probs = { 0.1, 0.2, 0.4, 0.1, 0.1,0.0,0.0,0.0,0.1 };  // A bell-shaped probability mass function with outlier
			DynaPlex::VarGroup vg;
			vg.Add("type", "custom");
			vg.Add("probs", probs);
			vg.Add("offset", 0);  // This means the values are [0, 1, 2, 3, 4]
			DiscreteDist customDist(vg);


			auto buckets = GatherBucketedSampleMeans(customDist, numSamples, numMeans);
			

			size_t withinOneSD = buckets[0] + buckets[-1];
			size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
			size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

			EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.05);
			EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.03);
			EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01);
		}

		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "geometric");
			vg.Add("mean", 4);
			DiscreteDist customDist(vg);


			auto buckets = GatherBucketedSampleMeans(customDist, numSamples, numMeans);


			size_t withinOneSD = buckets[0] + buckets[-1];
			size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
			size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

			EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.05);
			EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.03);
			EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01);
		}
		// You can then perform assertions on the bucketedMeans, e.g., 
		// expecting a certain number of means to lie within specific buckets.
	}



	TEST(discretedist, fractile) {
		// Let's consider a simple discrete distribution for the test
		std::vector<double> probs = { 0.1, 0.2, 0.4, 0.2, 0.1 };  // A bell-shaped probability mass function
		DynaPlex::VarGroup vg;
		vg.Add("type", "custom");
		vg.Add("probs", probs);
		vg.Add("offset", -1);  // This means the values are [-1, 0, 1, 2, 3]
		DiscreteDist customDist(vg);

		const double epsilon = 1e-6;

		ASSERT_EQ(customDist.Fractile(0.0), customDist.Min());  // Fractile of 1.0 should be the maximum value of the distribution


		// Testing various fractile levels adjusted with epsilon
		ASSERT_EQ(customDist.Fractile(0.1 - epsilon), -1);
		ASSERT_EQ(customDist.Fractile(0.3 - epsilon), 0);
		ASSERT_EQ(customDist.Fractile(0.5), 1);
		ASSERT_EQ(customDist.Fractile(0.7 - epsilon), 1);
		ASSERT_EQ(customDist.Fractile(0.9 - epsilon), 2);
		ASSERT_EQ(customDist.Fractile(0.9 + epsilon), 3);

		ASSERT_EQ(customDist.Fractile(1.0), customDist.Max());  // Fractile of 1.0 should be the maximum value of the distribution

		// You can add more sophisticated distributions or edge cases as needed.
	}


	TEST(discretedist, advanced_tests) {

		RNG rng{ 1234 };
		// Test for iterator
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "geometric");
			vg.Add("mean", 3.0);
			DiscreteDist dist(vg);
			double sum = 0.0;
			for (const auto& [qty, prob] : dist) {
				ASSERT_GE(qty, 0);  // Geometric is zero-based, so quantity should be non-negative.
				sum += prob;
			}
			ASSERT_NEAR(sum, 1.0, 0.001); // Total probability should be close to 1.
		}

		// Test for Add function with non-constant distributions
		{
			DynaPlex::VarGroup vg1, vg2;
			vg1.Add("type", "geometric");
			vg1.Add("mean", 3.0);
			DiscreteDist dist1(vg1);

			vg2.Add("type", "poisson");
			vg2.Add("mean", 4.0);
			DiscreteDist dist2(vg2);

			dist2 = dist2.Add(DiscreteDist::GetCustomDist(std::vector<double>{ 0.5, 0.25, 0.25 }));

			DiscreteDist result = dist1.Add(dist2);

			// Check that the expectation of the added distribution is roughly the sum of the individual expectations.
			double expectedValue = dist1.Expectation() + dist2.Expectation();
			ASSERT_NEAR(result.Expectation(), expectedValue, 0.001);

			// Check the probability of the sum being zero.
			double probZeroResult = dist1.ProbabilityAt(dist1.Min()) * dist2.ProbabilityAt(dist2.Min());
			ASSERT_NEAR(result.ProbabilityAt(result.Min()), probZeroResult, 0.001);

			// Check that the result is a valid probability mass function.
			double sum = 0.0;
			for (const auto& [qty, prob] : result) {
				sum += prob;
			}
			ASSERT_NEAR(sum, 1.0, 0.001); // Total probability should be close to 1.
		}


		// Test for Add function with non-constant distributions
		{
			DynaPlex::VarGroup vg1, vg2;
			vg1.Add("type", "geometric");
			vg1.Add("mean", 3.0);
			DiscreteDist dist1(vg1);

			vg2.Add("type", "custom");
			vg2.Add("probs", std::vector<double>{0.5, 0.25,0.25});
			DiscreteDist dist2(vg2);

			auto constant = DiscreteDist::GetConstantDist(-2);

			dist1 = dist1.Add(constant);
			dist2 = dist2.Add(DiscreteDist::GetConstantDist(-3));


			DiscreteDist result = dist1.Add(dist2);

			// Check that the expectation of the added distribution is roughly the sum of the individual expectations.
			double expectedValue = dist1.Expectation() + dist2.Expectation();
			ASSERT_NEAR(result.Expectation(), expectedValue, 0.001);

			double expectedVariance = dist1.Variance() + dist2.Variance();
			ASSERT_NEAR(result.Variance(), expectedVariance, 0.001);


			ASSERT_LE(result.Entropy(), dist1.Entropy() + dist2.Entropy());

			// Check the probability of the sum being zero.
			double probZeroResult = dist1.ProbabilityAt(dist1.Min()) * dist2.ProbabilityAt(dist2.Min());
			ASSERT_NEAR(result.ProbabilityAt(result.Min()), probZeroResult, 0.001);

			// Check that the result is a valid probability mass function.
			double sum = 0.0;
			for (const auto& [qty, prob] : result) {
				sum += prob;
			}
			ASSERT_NEAR(sum, 1.0, 0.001); // Total probability should be close to 1.
		}

		// Test for GetSample function
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "constant");
			vg.Add("value", 7);
			DiscreteDist dist(vg);

			// Since the distribution is constant, any sample we get should be the same value.
			for (int i = 0; i < 10; ++i) {
				ASSERT_EQ(dist.GetSample(rng), 7);
			}
		}
	}

	TEST(discretedist, VarGroupConstructor) {

	
		// Test Constant Distribution
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "constant");
			vg.Add("value", int64_t(10));

			DiscreteDist dist(vg);
			ASSERT_EQ(dist.Expectation(), 10.0);
			ASSERT_EQ(dist.Variance(), 0.0);
		}

		// Test Zero Distribution
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "zero");

			DiscreteDist dist(vg);
			ASSERT_EQ(dist.Expectation(), 0.0);
			ASSERT_EQ(dist.Variance(), 0.0);
		}

		// Test Poisson Distribution
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "poisson");
			vg.Add("mean", 5.0);

			DiscreteDist dist(vg);
			// Given that Poisson mean is lambda and variance is lambda:
			ASSERT_NEAR(dist.Expectation(), 5.0, 0.00001);
			ASSERT_NEAR(dist.Variance(), 5.0, 0.00001);
		}

		// Test Geometric Distribution (zero-based)
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "geometric");
			vg.Add("mean", 4.0);

			DiscreteDist dist(vg);

			// Given a mean m for a zero-based geometric distribution, p = 1/(m + 1)
			double p = 1.0 / (4.0 + 1.0);
			double expectedVariance = (1 - p) / (p * p);

			ASSERT_NEAR(dist.Expectation(), 4.0, 0.00001);
			ASSERT_NEAR(dist.Variance(), expectedVariance, 0.00001);
		}

		// Test Custom Distribution
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "custom");
			vg.Add("probs", std::vector<double>{0.1, 0.2, 0.3, 0.4});
			vg.Add("offset", int64_t(3));

			DiscreteDist dist(vg);
			// Validate PMF elements and other properties of the custom distribution.
			// For the sake of this example, I'll just check the range (Min and Max).
			ASSERT_EQ(dist.Min(), 3);
			ASSERT_EQ(dist.Max(), 6);
		}

		// Test for asking for type that is unavailable
		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "someunavailabletype");

			try {
				DiscreteDist dist(vg);
				FAIL(); // If we reach this line, it means no exception was thrown. The test should fail in that case.
			}
			catch (const DynaPlex::Error& e) {
				std::string expectedStart = "DynaPlex: DiscreteDist: Unknown type: someunavailabletype. Available ";
				ASSERT_TRUE(std::string(e.what()).compare(0, expectedStart.length(), expectedStart) == 0);
			}
		}
	}

	TEST(discretedist, inversion_maximum_tests) {
		
		// Construct a custom distribution
		std::vector<double> probs = { 0.1, 0.5, 0.3, 0.1 }; //With offset -1, these will be probabilities for -1, 0, 1, 2 respectively
		DynaPlex::VarGroup vg;
		vg.Add("type", "custom");
		vg.Add("probs", probs);
		vg.Add("offset", -1); // Setting the offset so that the first probability is -1
		DiscreteDist customDist(vg);

		// Test for Invert
		{
			DiscreteDist inverted = customDist.Invert();

			ASSERT_EQ(inverted.Min(), -2);
			ASSERT_EQ(inverted.Max(), 1);

			for (const auto& [qty, prob] : inverted) {
				ASSERT_EQ(prob, inverted.ProbabilityAt(qty));
			}


			// Check that the inverted distribution has the correct probabilities
			ASSERT_NEAR(inverted.ProbabilityAt(1), 0.1, 0.001);
			ASSERT_NEAR(inverted.ProbabilityAt(0), 0.5, 0.001);
			ASSERT_NEAR(inverted.ProbabilityAt(-1), 0.3, 0.001);
			ASSERT_NEAR(inverted.ProbabilityAt(-2), 0.1, 0.001);
		}

		// Test for TakeMaximumWith
		{
			int64_t threshold = 0;
			DiscreteDist maximumDist = customDist.TakeMaximumWith(threshold);

			// Check that the resulting distribution has the correct probabilities
			// Probabilities for max(-1, 0), max(0, 0), max(1, 0), max(2, 0) are 0.6, 0.5, 0.3, 0.1 respectively
			ASSERT_NEAR(maximumDist.ProbabilityAt(0), 0.6, 0.001);
			ASSERT_NEAR(maximumDist.ProbabilityAt(1), 0.3, 0.001);
			ASSERT_NEAR(maximumDist.ProbabilityAt(2), 0.1, 0.001);
		}
	}

} // namespace DynaPlex::Tests
