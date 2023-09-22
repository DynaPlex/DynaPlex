#pragma once
#include <vector>
#include <dynaplex/vargroup.h>
#include <dynaplex/rng.h>

namespace DynaPlex
{
	class DiscreteDist
	{
	public:
		struct QtyProb
		{
			int64_t qty;
			double prob;

		};

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
		static DiscreteDist GetConstantDist(int64_t value);
		static DiscreteDist GetZeroDist();
		static DiscreteDist GetPoissonDist(double mean);
		static DiscreteDist GetGeometricDist(double mean);
		/// <summary>
		/// Example: GetCumtomDist with probs = {0.1 0.2, 0.3, 0.4} and offset = -2 
		/// corresponds to -2 with prob 0.1, -1 with prob 0.2, 0 with prob 0.3 and 1 with prob 0.4.
		/// </summary>
		static DiscreteDist GetCustomDist(const std::vector<double>& probs, int64_t offset = 0);
		/// <summary>
		/// Example: GetCumtomDist with probs = {0.1 0.2, 0.3, 0.4} and offset = -2 
		/// corresponds to -2 with prob 0.1, -1 with prob 0.2, 0 with prob 0.3 and 1 with prob 0.4.
		/// </summary>
		static DiscreteDist GetCustomDist(std::vector<double>&& probs, int64_t offset = 0);

		int64_t Max() const
		{
			return min + static_cast<int64_t>(translatedPMF.size()) - 1ll;
		}
		int64_t Min() const
		{
			return min;
		}


		// returns distribution that corresponds to the sum of this and another distribution (assuming the two distributions are independent)
		DiscreteDist Add(const DiscreteDist& other) const;

		// Returns a discrete distribution that represents the maximum of the current distribution and the given value.
		DiscreteDist TakeMaximumWith(int64_t value) const;

		// Returns a discrete distribution that represents minus the current distribution.
		DiscreteDist Invert() const;
		/**
		 * Computes the fractile (quantile) for the given discrete distribution.
		 *
		 * The function returns the smallest integer value `i` such that the
		 * cumulative probability of the random variable being less than or
		 * equal to `i` is approximately equal to the provided `alpha`.
		 */
		int64_t Fractile(double alpha) const;
		double ProbabilityAt(int64_t value) const;
		double Expectation() const;
		double Variance() const;
		double StandardDeviation() const;
		double Entropy() const;

		DiscreteDist();

		DiscreteDist(const DynaPlex::VarGroup& vars);

		int64_t GetSample(DynaPlex::RNG& rng) const;

	};

} // namespace DynaPlex::Modelling