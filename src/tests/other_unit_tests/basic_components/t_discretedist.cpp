#include <iostream>
#include <deque>
#include <gtest/gtest.h>
#include <vector>
#include <iterator>
#include "dynaplex/error.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/rng.h"
#include "dynaplex/policycomparison.h"
#include <numeric>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/negative_binomial.hpp>
namespace DynaPlex::Tests {

	using namespace DynaPlex;

	template<typename SampleFunc>
	std::map<int64_t, size_t> GatherBucketedSampleMeans(double mean, double sigma, size_t numSamples, size_t numMeans, SampleFunc sampleFunc) {
		DynaPlex::RNG rng{false, 26071983 ,1234 }; // Using a fixed seed for reproducibility

		double trueMean = mean;
		double trueVariance = sigma * sigma;
		double sdOfSampleMean = std::sqrt(trueVariance / numSamples);

		std::map<int64_t, size_t> buckets; // Map from bucket index to count of means

		for (size_t meanIndex = 0; meanIndex < numMeans; ++meanIndex) {
			std::vector<double> vec;
			vec.reserve(numSamples);

			for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
				vec.push_back(sampleFunc(rng));
			}

			auto comparison = PolicyComparison::GetComparison(vec);

			double deviationFromTrueMean = comparison.mean(0) - trueMean;

			// Assigning the deviation to a bucket using the standard deviation of the sample mean
			int64_t bucketIndex = static_cast<int64_t>(std::floor(deviationFromTrueMean / sdOfSampleMean));
			buckets[bucketIndex]++;
		}

		return buckets;
	}


	std::map<int64_t, size_t> GatherBucketedSampleMeans(const DiscreteDist& dist, size_t numSamples, size_t numMeans) {
		DynaPlex::RNG rng(false,26071983); // Using a fixed seed for reproducibility

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
	TEST(discretedist, sampling)
	{
		int numMeans = 250;
		int numSamples = 250;
		{
			for (double mean : { 0.1/*, 5.0, 200.0*/, 500.0})
			{
				double sigma = std::sqrt(mean);
				auto buckets = GatherBucketedSampleMeans(mean, sigma, numSamples, numMeans,
					[mean, sigma](DynaPlex::RNG& rng) -> double {
						return DiscreteDist::GetPoissonSample(mean, rng);
					});


				size_t withinOneSD = buckets[0] + buckets[-1];
				size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
				size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

				EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.08) << "pois " << mean;
				EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.04) << "pois " << mean;
				EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01) << "pois " << mean;
			}
		}

		{
			for (double mean_in : { 0.1/*, 5.0, 200.0*/, 500.0})
			{
				auto dist = DiscreteDist::GetGeometricDist(mean_in);
				auto mean = dist.Expectation();
				double sigma = dist.StandardDeviation();
				auto buckets = GatherBucketedSampleMeans(mean, sigma, numSamples, numMeans,
					[mean, sigma](DynaPlex::RNG& rng) -> double {
						return DiscreteDist::GetGeometricSample(mean, rng);
					});


				size_t withinOneSD = buckets[0] + buckets[-1];
				size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
				size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

				EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.08) << "geom " << mean;
				EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.04) << "geom " << mean;
				EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01) << "geom " << mean;
			}
		}

		{
			for (double p : { 0.1/*, 0.5, 0.9*/})
			{
				for (int64_t n : {1,/*10,*/ 50})
				{
					auto dist = DiscreteDist::GetBinomialDist(n, p);
					auto mean = dist.Expectation();
					double sigma = dist.StandardDeviation();
					auto buckets = GatherBucketedSampleMeans(mean, sigma, numSamples, numMeans,
						[n, p](DynaPlex::RNG& rng) -> double {
							return DiscreteDist::GetBinomialSample(n, p, rng);
						});

					size_t withinOneSD = buckets[0] + buckets[-1];
					size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
					size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

					EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.08) << " bin  p - " << p << " n " << n;
					EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.04) << " bin  p - " << p << " n " << n;
					EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01) << " bin  p - " << p << " n " << n;
				}
			}
		}

		{
			for (double p : { 0.1, 0.5, 0.9})
			{
				for (int64_t r : {2/*, 10, 50*/})
				{
					auto dist = DiscreteDist::GetNegativeBinomialDist(r, p);
					auto mean = dist.Expectation();
					double sigma = dist.StandardDeviation();
					auto buckets = GatherBucketedSampleMeans(mean, sigma, numSamples, numMeans,
						[r, p](DynaPlex::RNG& rng) -> double {
							return DiscreteDist::GetNegativeBinomialSample(r, p, rng);
						});

					size_t withinOneSD = buckets[0] + buckets[-1];
					size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
					size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

					EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.1) << " bin  p - " << p << " r " << r;
					EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.04) << " bin  p - " << p << " r " << r;
					EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01) << " bin  p - " << p << " r " << r;
				}
			}
		}


		{
			for (double mean : {0.08, 1.0, 10.0})
			{
				for (double sigma : {0.2, 2.0, 10.0})
				{
					if (sigma * sigma < DiscreteDist::LeastVarianceRequiredForAERFit(mean))
					{
						continue;
					}
					if (sigma / mean > 5 || mean / sigma > 4)
					{//these fail because of insufficient samples to get to normality
						continue;
					}

					auto buckets = GatherBucketedSampleMeans(mean, sigma, numSamples, numMeans,
						[mean, sigma](DynaPlex::RNG& rng) -> double {
							return DiscreteDist::GetAdanEenigeResingSample(mean, sigma, rng);
						});
					size_t withinOneSD = buckets[0] + buckets[-1];
					size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
					size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

					EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.08) << "  mean  " << mean << "  sigma " << sigma;
					EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.04) << "  mean  " << mean << "  sigma " << sigma;
					EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01) << "  mean  " << mean << "  sigma " << sigma;
				}
			}
		}

	}

	TEST(discretedist, GetSample) {


		int numMeans = 500;
		int numSamples = 500;
		{
			std::vector<double> probs = { 0.1, 0.2, 0.4, 0.1, 0.1,0.0,0.0,0.0,0.1 };  // A bell-shaped probability mass function with outlier
			DynaPlex::VarGroup vg;
			vg.Add("type", "custom");
			vg.Add("probs", probs);
			vg.Add("offset", 0);
			DiscreteDist customDist(vg);
			customDist.OptimizeForSampling();

			auto buckets = GatherBucketedSampleMeans(customDist, numSamples, numMeans);


			size_t withinOneSD = buckets[0] + buckets[-1];
			size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
			size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

			EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.08);
			EXPECT_NEAR(withinTwoSD, 0.9545 * numMeans, numMeans * 0.03);
			EXPECT_NEAR(withinThreeSD, 0.9973 * numMeans, numMeans * 0.01);
		}

		{
			DynaPlex::VarGroup vg;
			vg.Add("type", "geometric");
			vg.Add("mean", 4);
			DiscreteDist customDist(vg);
			customDist.OptimizeForSampling();
			

			auto buckets = GatherBucketedSampleMeans(customDist, numSamples, numMeans);


			size_t withinOneSD = buckets[0] + buckets[-1];
			size_t withinTwoSD = withinOneSD + buckets[1] + buckets[-2];
			size_t withinThreeSD = withinTwoSD + buckets[2] + buckets[-3];

			EXPECT_NEAR(withinOneSD, 0.6827 * numMeans, numMeans * 0.08);
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

	TEST(DiscreteDistTests, ConditionalSampleTest) {
		// Example setup: Create a geometric distribution with mean 5
		double mean = 5.0;
		DiscreteDist dist = DiscreteDist::GetGeometricDist(mean);

		// Create an RNG object
		DynaPlex::RNG rng{ false ,1203};

		// Specify the minimum value for conditional sampling
		int64_t minimum_value = 3;

		// Number of samples to generate for the test
		int num_samples = 1000;

		// Generate samples and check if they meet the condition
		for (int i = 0; i < num_samples; ++i) {
			int64_t sample = dist.GetConditionalSample(rng, minimum_value);
			ASSERT_GE(sample, minimum_value) << "Sample " << sample << " is less than the minimum value of " << minimum_value;
		}
	}

	void TestConditionalSampling(DiscreteDist& dist)
	{
		DynaPlex::RNG rng{ false ,1203 };

		EXPECT_THROW(dist.GetConditionalSample(rng, dist.Max() + 1), DynaPlex::Error);
		for (int64_t minimum_value = dist.Min() - 1; minimum_value <= dist.Max(); minimum_value++)
		{
			// Setup: Define a custom distribution. For simplicity, let's say it's a simple distribution
			// where the probabilities are known, and we can calculate the conditional mean analytically.
				// Create an RNG object

			// Specify the minimum value for conditional sampling and the expected conditional mean

			double mean{ 0.0 };
			double prob{ 0.0 };

			for (const auto& [qty, pr] : dist)
			{
				if (qty >= minimum_value)
				{
					mean += qty * pr;
					prob += pr;
				}
			}
			double expected_conditional_mean{ mean / prob };


			// Number of samples to generate for the test
			int num_samples = 10000;

			// Generate samples
			std::vector<int64_t> samples;
			for (int i = 0; i < num_samples; ++i) {
				samples.push_back(dist.GetConditionalSample(rng, minimum_value));
			}

			// Calculate the empirical mean of the conditional samples
			double empirical_mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();

			// Define a tolerance for comparing the means
			double tolerance = 0.04;

			// Assert that the empirical mean is within the tolerance of the expected mean
			ASSERT_NEAR(empirical_mean, expected_conditional_mean, empirical_mean* tolerance) << "The empirical mean of the conditional distribution is not within the expected range." << minimum_value;

		}
	}

	TEST(DiscreteDistTests, ConditionalMeanTest) {

		{
			std::vector<double> probabilities = { 0.2, 0.25, 0.15, 0.2, 0.1,0.0,0.1 }; // Example probabilities
			int64_t offset = -1; // Starting value of the distribution
			DiscreteDist dist = DiscreteDist::GetCustomDist(probabilities, offset);
			TestConditionalSampling(dist);
			dist.OptimizeForSampling();
			TestConditionalSampling(dist);
		}

		{
			std::vector<double> probabilities = { 0.2, 0.25, 0.15, 0.2, 0.1,0.0,0.1 }; // Example probabilities
			int64_t offset = -1; // Starting value of the distribution
			DiscreteDist dist = DiscreteDist::GetAdanEenigeResingDist(10,5);
			dist = dist.Add(DiscreteDist::GetConstantDist(-5));
			TestConditionalSampling(dist);
			dist.OptimizeForSampling();
			TestConditionalSampling(dist);
		}

	}


	TEST(discretedist, advanced_tests) {

		RNG rng(false, 1234 );
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


			double expectation{ 0.0 };
			auto qtyprobs = dist.QuantityProbabilities();
			for (auto& [qty, prob] : qtyprobs)
			{
				expectation += qty * prob;
			}
			ASSERT_NEAR(expectation, dist.Expectation(), 1.0e-10);


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
			vg2.Add("probs", std::vector<double>{0.5, 0.25, 0.25});
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

		// Test Binomial Distribution
		{
			DynaPlex::VarGroup vg;
			double n = 20, p = 0.3;
			vg.Add("type", "binomial");
			vg.Add("n", n);
			vg.Add("p", p);

			DiscreteDist dist;
			ASSERT_NO_THROW(
				//Binomial
				dist = DiscreteDist(vg);
			);
			boost::math::binomial_distribution<> boostDist(n, p);
			double expectedMean = boost::math::mean(boostDist);
			double expectedVariance = boost::math::variance(boostDist);

			ASSERT_NEAR(dist.Expectation(), expectedMean, 0.00001);
			ASSERT_NEAR(dist.Variance(), expectedVariance, 0.00001);
		}

		// Test Negative Binomial Distribution
		{
			double successes = 10;
			double prob = 0.4;
			DynaPlex::VarGroup vg;
			vg.Add("type", "negative_binomial");
			vg.Add("r", successes);
			vg.Add("p", prob);

			DiscreteDist dist;
			ASSERT_NO_THROW(
				dist = DiscreteDist(vg);
			);
			boost::math::negative_binomial_distribution<> boostDist(successes, prob);
			double expectedMean_NB = boost::math::mean(boostDist);
			double expectedVariance_NB = boost::math::variance(boostDist);

			ASSERT_NEAR(dist.Expectation(), expectedMean_NB, 0.00001);
			ASSERT_NEAR(dist.Variance(), expectedVariance_NB, 0.00001);
		}

		{
			for (double mean_input = 0.0; mean_input < 10.0; mean_input += 1.0)
			{
				for (double eps : { 0.0, 1e-9})
				{
					double mean = mean_input + eps;
					if (mean == 0.0)
						continue;
					for (double variance : {1e-8, 1e-6, 1e-4})
					{
						double stdev = std::sqrt(variance);

						DynaPlex::VarGroup vg;
						vg.Add("type", "adan_eenige_resing");
						vg.Add("mean", mean);
						vg.Add("stdev", stdev);

						DiscreteDist dist;
						ASSERT_NO_THROW(
							dist = DiscreteDist(vg);
						) << " mean " << mean << " var " << variance << std::endl;
						ASSERT_NEAR(dist.Expectation(), mean, std::max(1e-8, 1e-6 * mean)) << " mean " << mean << " var " << variance << std::endl;
						//it seems there are some numerical issues around variance=mean
						//if we use poisson, it gives the current tolerance.
						//if we use negbin - issue is that it is slow.
						//Hence the relatively wide tolerance here:
						ASSERT_NEAR(dist.StandardDeviation(), stdev, std::max(1e-3, 1e-3 * stdev)) << " mean " << mean << " var " << variance << std::endl;;
					}
				}
			}
		}


		{
			for (double mean_input = 0.001; mean_input < 1000.0; mean_input *= 1.5)
			{
				double mean = mean_input;
				for (double extra : {-1.0, 0.00000001, 0.000001, 0.0001, 0.001, 0.01, 0.1, 1.0, 2.0, 4.0, 8.0, mean, mean * 2, mean * 10})
				{

					double leastVarianceRequired = DiscreteDist::LeastVarianceRequiredForAERFit(mean);

					double variance;
					if (extra == -1.0)
					{
						variance = mean;
					}
					else
					{
						variance = leastVarianceRequired + extra;
					}
					double stdev = std::sqrt(variance);

					DynaPlex::VarGroup vg;
					vg.Add("type", "adan_eenige_resing");
					vg.Add("mean", mean);
					vg.Add("stdev", stdev);

					DiscreteDist dist;
					ASSERT_NO_THROW(
						dist = DiscreteDist(vg);
					) << " mean " << mean << " var " << variance << std::endl;

					ASSERT_NEAR(dist.Expectation(), mean, std::max(1e-8, 1e-5 * mean)) << " mean " << mean << " var " << variance << std::endl;
					//it seems there are some numerical issues around variance=mean
					//if we use poisson, it gives the current tolerance.
					//if we use negbin - issue is that it is slow.
					//Hence the relatively wide tolerance here:
					ASSERT_NEAR(dist.StandardDeviation(), stdev, std::max(1e-8, 1e-3 * stdev)) << " mean " << mean << " var " << variance << std::endl;;
				}
			}


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

	TEST(discretedist, mix_tests) {

		// Construct two custom distributions
		std::vector<double> probs1 = { 0.2, 0.5, 0.3 }; // Probabilities for 0, 1, 2 respectively
		std::vector<double> probs2 = { 0.4, 0.4, 0.2 }; // Probabilities for 0, 1, 2 respectively
		DynaPlex::VarGroup vg1, vg2;
		vg1.Add("type", "custom");
		vg1.Add("probs", probs1);
		vg2.Add("type", "custom");
		vg2.Add("probs", probs2);

		DiscreteDist dist1(vg1);
		DiscreteDist dist2(vg2);

		// Probability of the second distribution in the mix
		double probOfSecond = 0.3;

		// Mix the two distributions
		DiscreteDist mixedDist = dist1.Mix(dist2, probOfSecond);

		// The mixed probabilities should be: 
		// 0.2*(1-0.3) + 0.4*0.3 for 0, 
		// 0.5*(1-0.3) + 0.4*0.3 for 1,
		// 0.3*(1-0.3) + 0.2*0.3 for 2.

		ASSERT_NEAR(mixedDist.ProbabilityAt(0), 0.2 * (1 - probOfSecond) + 0.4 * probOfSecond, 0.001);
		ASSERT_NEAR(mixedDist.ProbabilityAt(1), 0.5 * (1 - probOfSecond) + 0.4 * probOfSecond, 0.001);
		ASSERT_NEAR(mixedDist.ProbabilityAt(2), 0.3 * (1 - probOfSecond) + 0.2 * probOfSecond, 0.001);

		// Check the range of the mixed distribution
		ASSERT_EQ(mixedDist.Min(), 0);
		ASSERT_EQ(mixedDist.Max(), 2);

		// Confirm that the probabilities sum up to 1
		double totalProb = 0;
		for (const auto& [qty, prob] : mixedDist) {
			totalProb += prob;
		}
		ASSERT_NEAR(totalProb, 1.0, 0.001);
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