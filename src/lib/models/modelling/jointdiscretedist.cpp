#include "dynaplex/modelling/jointdiscretedist.h"
#include "dynaplex/error.h"

namespace DynaPlex
{

	int64_t JointDiscreteDist::DistinctValueCount() const {
		return static_cast<int64_t>( translatedPMF.size());
	}

	std::vector<JointDiscreteDist::QtyProb> JointDiscreteDist::QuantityProbabilities() const {
		std::vector<JointDiscreteDist::QtyProb> quantityProbabilities;
		quantityProbabilities.reserve(DistinctValueCount());
		for (auto qtyprob : *this)
		{
			quantityProbabilities.push_back(qtyprob);
		}
		return quantityProbabilities;
	}

	JointDiscreteDist::JointDiscreteDist() : JointQtys({ { 0, 0 } }), translatedPMF({ 1.0 })
	{
	}

	JointDiscreteDist::Iterator::Iterator(const JointDiscreteDist& dist, std::size_t index)
		: dist_(dist), index_(index) {}

	JointDiscreteDist::QtyProb JointDiscreteDist::Iterator::operator*() const {
		return { static_cast<int64_t>(index_), dist_.translatedPMF[index_] };
	}

	JointDiscreteDist::Iterator& JointDiscreteDist::Iterator::operator++() {
		++index_;
		return *this;
	}

	JointDiscreteDist::Iterator JointDiscreteDist::Iterator::operator++(int) {
		Iterator tmp(*this);
		++index_;
		return tmp;
	}

	bool JointDiscreteDist::Iterator::operator==(const Iterator& other) const {
		return &dist_ == &other.dist_ && index_ == other.index_;
	}

	bool JointDiscreteDist::Iterator::operator!=(const Iterator& other) const {
		return !(*this == other);
	}

	JointDiscreteDist::Iterator JointDiscreteDist::begin() const {
		return Iterator(*this, 0);
	}

	JointDiscreteDist::Iterator JointDiscreteDist::end() const {
		return Iterator(*this, translatedPMF.size());
	}

	JointDiscreteDist::JointDiscreteDist(const DiscreteDist& dist1, const DiscreteDist& dist2)
		: JointDiscreteDist(std::vector<DiscreteDist>{dist1, dist2}) {
	}

	JointDiscreteDist::JointDiscreteDist(const std::vector<DynaPlex::DiscreteDist>& distributions)
	{
		if (distributions.size() < 2) {
			throw DynaPlex::Error("JointDiscretDist: provide at least two discrete distibutions to create a joint distribution.");
		}

		std::vector<double> probVec;
		std::vector<std::vector<int64_t>> Qtys;
		GetJointDistribution(distributions, 0, Qtys, probVec, {}, 1.0);

		if (!IsProbMassFunction(probVec))
		{
			throw DynaPlex::Error("JointDiscretDist: Probabilities should be nonnegative and sum to 1.0");
		}

		*this = JointDiscreteDist(std::move(probVec), std::move(Qtys));
	}

	void JointDiscreteDist::GetJointDistribution(const std::vector<DynaPlex::DiscreteDist>& distributions,
		size_t currentDistIndex,
		std::vector<std::vector<int64_t>>& Qtys,
		std::vector<double>& probVec,
		std::vector<int64_t> currentQtys,
		double currentProb) {

		if (currentDistIndex == distributions.size()) {
			// Base case: all distributions processed
			if (currentProb > epsilon) {
				Qtys.push_back(currentQtys);
				probVec.push_back(currentProb);
			}
			return;
		}

		for (const auto& [qty, prob] : distributions[currentDistIndex]) {
			std::vector<int64_t> newQtys = currentQtys;
			newQtys.push_back(qty);
			double newProb = currentProb * prob;
			GetJointDistribution(distributions, currentDistIndex + 1, Qtys, probVec, newQtys, newProb);
		}
	}

	bool JointDiscreteDist::IsProbMassFunction(const std::vector<double>& PMF)
	{
		auto copy = PMF;
		//Sorts in ascending order for numerical stability.
		std::sort(copy.begin(), copy.end());
		if (copy[0] < 0.0)
		{//negative probabilities are not allowed.
			return false;
		}
		double TotalProb = 0.0;
		for (const auto& d : copy)
		{
			TotalProb += d;
		}
		return std::abs(TotalProb - 1.0) < 1e-8;
	}

	JointDiscreteDist::JointDiscreteDist(std::vector<double>&& TranslatedProbMF, std::vector<std::vector<int64_t>>&& JointQtys)
		: translatedPMF(std::move(TranslatedProbMF)), JointQtys(std::move(JointQtys))
	{
	}

	JointDiscreteDist::JointDiscreteDist(const std::vector<double>& TranslatedProbMF, const std::vector<std::vector<int64_t>>& JointQtys)
		: translatedPMF(TranslatedProbMF), JointQtys(JointQtys)
	{
	}

	double JointDiscreteDist::ProbabilityAt(int64_t value) const
	{
		if (value < 0 || value > Max())
		{
			return 0.0;
		}
		return translatedPMF[value];
	}

	int64_t JointDiscreteDist::GetSample(DynaPlex::RNG& rng) const {
		// Generate a uniform random number between 0 and 1
		double randomValue = rng.genUniform();
		double cumulativeProbability = 0.0;
		for (size_t i = 0; i < translatedPMF.size(); i++) {
			cumulativeProbability += translatedPMF[i];
			if (randomValue < cumulativeProbability) {
				return static_cast<int64_t>(i);
			}
		}

		// This should technically never be reached if probabilities sum to 1, 
		// but it handles potential numerical inaccuracies.
		return Max();
	}

	std::vector<int64_t> JointDiscreteDist::GetSampleQtys(DynaPlex::RNG& rng) const {
		// Generate a uniform random number between 0 and 1
		double randomValue = rng.genUniform();
		double cumulativeProbability = 0.0;
		for (size_t i = 0; i < translatedPMF.size(); i++) {
			cumulativeProbability += translatedPMF[i];
			if (randomValue < cumulativeProbability) {
				return GetQtysForJointDist(static_cast<int64_t>(i));
			}
		}

		// This should technically never be reached if probabilities sum to 1, 
		// but it handles potential numerical inaccuracies.
		return GetQtysForJointDist(Max());
	}

	std::vector<int64_t> JointDiscreteDist::GetQtysForJointDist(int64_t pos) const {
		if (pos >= JointQtys.size() || pos < 0)
		{
			throw DynaPlex::Error("JointDiscreteDist: vector length error for JointQty.");
		}
		return JointQtys[pos];
	}

	int64_t JointDiscreteDist::FindPositionInJointQtys(const std::vector<int64_t> qty) const {
		for (int64_t i = 0; i < JointQtys.size(); ++i) {
			if (JointQtys[i] == qty) {
				return i;  // Return the index if found
			}
		}

		// Throw an exception if the element is not found
		throw DynaPlex::Error("JointDiscreteDist: joint quantities not found in JointQtys.");
	}

	double JointDiscreteDist::ProbabilityAtFromQtys(const std::vector<int64_t> qty) const
	{
		int64_t pos = FindPositionInJointQtys(qty);
		return translatedPMF[pos];
	}

}