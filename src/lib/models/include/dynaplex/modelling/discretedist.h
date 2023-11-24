#pragma once
#include <vector>
#include <dynaplex/vargroup.h>
#include <dynaplex/rng.h>

namespace DynaPlex
{
	class DiscreteDist
	{
	public:
		
		using QtyProb = std::tuple<int64_t, double>;
		
		using value_type = QtyProb;
		using size_type = std::size_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;

		// Iterator class for DiscreteDist, for enabling to iterate over instances of DiscreteDist and be returned a QtyProb
		class Iterator {

			using value_type = QtyProb;
			using difference_type = std::ptrdiff_t;
			using pointer = const value_type*;
			using reference = const value_type&;
			using iterator_category = std::forward_iterator_tag;

			const DiscreteDist& dist_;
			std::size_t index_;
		public:
			Iterator(const DiscreteDist& dist, std::size_t index);
			QtyProb operator*() const;
			Iterator& operator++();
			Iterator operator++(int);
			bool operator==(const Iterator& other) const;
			bool operator!=(const Iterator& other) const;
		};
		Iterator begin() const;
		Iterator end() const;

		bool operator==(const DiscreteDist& other) const = default;
	
	private:
		std::vector<double> translatedPMF{};
		int64_t min{ 0 };




		inline static double epsilon{ 1e-16 };


		static void Trim(std::vector<double>& toBeTrimmed, int64_t& min);


		// Always ensure the PMF is validated using IsProbMassFunction 
		// before constructing a DiscreteDist object.
		DiscreteDist(const std::vector<double>& translatedPMF, int64_t offset);
		DiscreteDist(std::vector<double>&& translatedPMF, int64_t offset);


		static bool IsProbMassFunction(const std::vector<double>& PMF);

		
	public:
		
		/// returns the least variance required for a certain mean, such that AER can be succesfully fit.
		static double LeastVarianceRequiredForAERFit(double mean);


		static DiscreteDist GetAdanEenigeResingDist(double mean, double stdev);
		static DiscreteDist GetBinomialDist(double n, double p);
		static DiscreteDist GetNegativeBinomialDist(double r, double p);
		static DiscreteDist GetConstantDist(int64_t value);
		static DiscreteDist GetZeroDist();
		static DiscreteDist GetPoissonDist(double mean);
		static DiscreteDist GetGeometricDist(double mean);
		static DiscreteDist GetGeometricDistFromProb(double p);


		static int64_t GetAdanEenigeResingSample(double mean, double stdev, DynaPlex::RNG&);
		static int64_t GetBinomialSample(int64_t n, double p, DynaPlex::RNG&);
		static int64_t GetNegativeBinomialSample(int64_t r, double p, DynaPlex::RNG&);
		static int64_t GetPoissonSample(double mean, DynaPlex::RNG&);
		static int64_t GetGeometricSample(double mean, DynaPlex::RNG&);
		static int64_t GetGeometricSampleFromProb(double p, DynaPlex::RNG&);


		/**
		 * Example: GetCustomDist with probs = {0.1 0.2, 0.3, 0.4} and offset = -2 
		 * corresponds to -2 with prob 0.1, -1 with prob 0.2, 0 with prob 0.3 and 1 with prob 0.4.
		 */
		static DiscreteDist GetCustomDist(const std::vector<double>& probs, int64_t offset = 0);
			

		/**
		 * Example: GetCustomDist with probs = {0.1 0.2, 0.3, 0.4} and offset = -2
		 * corresponds to -2 with prob 0.1, -1 with prob 0.2, 0 with prob 0.3 and 1 with prob 0.4.
		 */
		static DiscreteDist GetCustomDist(std::vector<double>&& probs, int64_t offset = 0);

		int64_t Max() const
		{
			return min + static_cast<int64_t>(translatedPMF.size()) - 1ll;
		}
		int64_t Min() const
		{
			return min;
		}
		
		/**Returns a distribution that corresponds to a mix of this distribution (with probability 1.0-prob_of_other)
		 * and the other distribution, with probability prob_of_other. 
		 */
		DiscreteDist Mix(const DiscreteDist& other, double prob_of_other) const;


		/// returns distribution that corresponds to the sum of this and another distribution (assuming the two distributions are independent)
		DiscreteDist Add(const DiscreteDist& other) const;

		/// Returns a discrete distribution that represents the maximum of the current distribution and the given value.
		DiscreteDist TakeMaximumWith(int64_t value) const;

		/// Returns a discrete distribution that represents minus the current distribution.
		DiscreteDist Invert() const;
		/**
		 * Computes the fractile (quantile) for the given discrete distribution.
		 *
		 * The function returns the smallest integer value `i` such that the
		 * cumulative probability of the random variable being less than or
		 * equal to `i` is approximately equal to the provided `alpha`.
		 */
		int64_t Fractile(double alpha) const;
		/**
		 * Returns the probability that the rv takes on this specific value. 
		 */
		double ProbabilityAt(int64_t value) const;
		///Returns expectation of the rv. 
		double Expectation() const;
		///Returns variance of the rv.
		double Variance() const;
		///Returns standard deviation of the rv.
		double StandardDeviation() const;
		///Returns entropy of the rv.
		double Entropy() const;
		///returns the values for which probabilities are stored in this r.v.. Note some of these probabilities may be zero.
		int64_t DistinctValueCount() const;

		//Gets a vector that contains pairs of quantities and probabilities, together representing this probability distribution. 
		std::vector<QtyProb> QuantityProbabilities() const;

		DiscreteDist();

		DiscreteDist(const DynaPlex::VarGroup& vars);
		/// Returns a sample of the rv, using the rng as random number generator. 
		int64_t GetSample(DynaPlex::RNG& rng) const;

	};

} // namespace DynaPlex::Modelling