#pragma once
#include <vector>
#include <dynaplex/vargroup.h>
#include <dynaplex/rng.h>
#include "dynaplex/modelling/discretedist.h"

namespace DynaPlex
{
	class JointDiscreteDist
	{
	public:
		
		using QtyProb = std::tuple<int64_t, double>;
		
		using value_type = QtyProb;
		using size_type = std::size_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;

		// Iterator class for JointDiscreteDist, for enabling to iterate over instances of JointDiscreteDist and be returned a QtyProb
		class Iterator {

			using value_type = QtyProb;
			using difference_type = std::ptrdiff_t;
			using pointer = const value_type*;
			using reference = const value_type&;
			using iterator_category = std::forward_iterator_tag;

			const JointDiscreteDist& dist_;
			std::size_t index_;
		public:
			Iterator(const JointDiscreteDist& dist, std::size_t index);
			QtyProb operator*() const;
			Iterator& operator++();
			Iterator operator++(int);
			bool operator==(const Iterator& other) const;
			bool operator!=(const Iterator& other) const;
		};
		Iterator begin() const;
		Iterator end() const;

		bool operator==(const JointDiscreteDist& other) const = default;
	
	private:
		std::vector<double> translatedPMF{};
		std::vector<std::vector<int64_t>> JointQtys;


		inline static double epsilon{ 1e-16 };


		// Always ensure the PMF is validated using IsProbMassFunction 
		// before constructing a JointDiscreteDist object.
		JointDiscreteDist(std::vector<double>&& TranslatedProbMF, std::vector<std::vector<int64_t>>&& JointQyt);
		JointDiscreteDist(const std::vector<double>& translatedPMF, const std::vector<std::vector<int64_t>>& JointQyt);

		static bool IsProbMassFunction(const std::vector<double>& PMF);

		static void GetJointDistribution(const std::vector<DynaPlex::DiscreteDist>& distributions,
			size_t currentDistIndex,
			std::vector<std::vector<int64_t>>& Qtys,
			std::vector<double>& probVec,
			std::vector<int64_t> currentQtys,
			double currentProb);
		
	public:

		/**
		 * Returns the probability that the rv takes on this specific value. 
		 */
		double ProbabilityAt(int64_t value) const;

		///returns the values for which probabilities are stored in this r.v.. Note some of these probabilities may be zero.
		int64_t DistinctValueCount() const;

		//Gets a vector that contains pairs of quantities and probabilities, together representing this probability distribution. 
		std::vector<QtyProb> QuantityProbabilities() const;

		JointDiscreteDist();

		JointDiscreteDist(const std::vector<DynaPlex::DiscreteDist>& distributions);

		JointDiscreteDist(const DiscreteDist& dist1, const DiscreteDist& dist2);

		int64_t Max() const
		{
			return static_cast<int64_t>(translatedPMF.size()) - 1ll;
		}

		/// Returns a sample of the rv, using the rng as random number generator. 
		int64_t GetSample(DynaPlex::RNG& rng) const;

		std::vector<int64_t> GetSampleQtys(DynaPlex::RNG& rng) const;

		std::vector<int64_t> GetQtysForJointDist(int64_t pos) const;

		int64_t FindPositionInJointQtys(const std::vector<int64_t> qty) const;

		double ProbabilityAtFromQtys(const std::vector<int64_t> qty) const;

		// it might be faster to get JointQtys beforehand have it as part of MDP instead of querrying the quantities after each event
		const std::vector<std::vector<int64_t>>& GetJointQtys() const
		{
			return JointQtys;
		}

	};

} // namespace DynaPlex::Modelling