#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/error.h"
#include <algorithm>  //sort
#include <cmath>  //atan
#include <unordered_map>


namespace DynaPlex
{
	
	enum class DistType {
		CONSTANT,
		ZERO,
		POISSON,
		GEOMETRIC,
		CUSTOM,
		UNKNOWN
	};

	int64_t DiscreteDist::DistinctValueCount() const {
		return static_cast<int64_t>( translatedPMF.size());
	}



	std::vector<DiscreteDist::QtyProb> DiscreteDist::QuantityProbabilities() const {
		std::vector<DiscreteDist::QtyProb> quantityProbabilities;
		quantityProbabilities.reserve(DistinctValueCount());
		for (auto qtyprob : *this)
		{
			quantityProbabilities.push_back(qtyprob);
		}
		return quantityProbabilities;
	}


	DiscreteDist::DiscreteDist(const DynaPlex::VarGroup& vars) 
	{
		std::string type;
		vars.Get("type", type);

		// Map from type string to the corresponding enum
		std::unordered_map<std::string, DistType> typeMap = {
			{"constant", DistType::CONSTANT},
			{"zero", DistType::ZERO},
			{"poisson", DistType::POISSON},
			{"geometric", DistType::GEOMETRIC},
			{"custom", DistType::CUSTOM}
		};

		DistType distType = typeMap.count(type) ? typeMap[type] : DistType::UNKNOWN;

		switch (distType) {
		case DistType::CONSTANT:
		{
			int64_t value;
			vars.Get("value", value);
			*this = GetConstantDist(value);
			break;
		}

		case DistType::ZERO:
		{
			*this = GetZeroDist();
			break;
		}

		case DistType::POISSON:
		case DistType::GEOMETRIC:
		{
			double mean;
			vars.Get("mean", mean);
			if (distType == DistType::POISSON)
				*this = GetPoissonDist(mean);
			else
				*this = GetGeometricDist(mean);
			break;
		}

		case DistType::CUSTOM:
		{
			std::vector<double> probs;
			vars.Get("probs", probs);
			
			int64_t offset;
			if (vars.HasKey("offset"))
			{
				vars.Get("offset", offset);
			}
			else
			{
				offset = 0;
			}
			*this = GetCustomDist(std::move(probs), offset);
			break;
		}

		case DistType::UNKNOWN:
		default:
		{
			// Build the error message with available distribution types using structured bindings
			std::string availableTypes = "Available distribution types are: ";
			for (const auto& [key, _] : typeMap) {
				availableTypes += key + ", ";
			}
			availableTypes = availableTypes.substr(0, availableTypes.size() - 2); // Removing trailing ", "

			throw DynaPlex::Error("DiscreteDist: Unknown type: " + type + ". " + availableTypes);
		}
		}
	}

	DiscreteDist::DiscreteDist() : min(0), translatedPMF({ 1.0 })
	{
	}

	static double PI = ::std::atan(1.0) * 4;



	DiscreteDist::Iterator::Iterator(const DiscreteDist& dist, std::size_t index)
		: dist_(dist), index_(index) {}

	DiscreteDist::QtyProb DiscreteDist::Iterator::operator*() const {
		return { dist_.Min() + static_cast<int64_t>(index_), dist_.translatedPMF[index_] };
	}

	DiscreteDist::Iterator& DiscreteDist::Iterator::operator++() {
		++index_;
		return *this;
	}

	DiscreteDist::Iterator DiscreteDist::Iterator::operator++(int) {
		Iterator tmp(*this);
		++index_;
		return tmp;
	}

	bool DiscreteDist::Iterator::operator==(const Iterator& other) const {
		return &dist_ == &other.dist_ && index_ == other.index_;
	}

	bool DiscreteDist::Iterator::operator!=(const Iterator& other) const {
		return !(*this == other);
	}

	DiscreteDist::Iterator DiscreteDist::begin() const {
		return Iterator(*this, 0);
	}

	DiscreteDist::Iterator DiscreteDist::end() const {
		return Iterator(*this, translatedPMF.size());
	}



	void DiscreteDist::Trim(std::vector<double>& ToBeTrimmed, int64_t& min)
	{   //trims the PMF to remove leading and trailing zeros and near-zeros. 
		if (!IsProbMassFunction(ToBeTrimmed))
		{
			throw DynaPlex::Error("DiscretDist: Probabilities should be nonnegative and sum to 1.0");
		}
		size_t iter = 0;
		while (ToBeTrimmed[iter] <= epsilon)
		{
			iter++;
		}
		size_t minPos = iter;

		iter = ToBeTrimmed.size() - 1;
		while (ToBeTrimmed[iter] < epsilon)
		{
			iter--;
		}
		size_t maxPos = iter;

		std::vector<double> TrimmedPMF{};
		TrimmedPMF.reserve(maxPos + 1 - minPos);

		for (size_t i = minPos; i <= maxPos; i++)
		{
			TrimmedPMF.push_back(ToBeTrimmed[i]);
		}
		min += static_cast<int64_t>(minPos);
		ToBeTrimmed = std::move(TrimmedPMF);
	}


	//Note the pass by value since we will sort..
	bool DiscreteDist::IsProbMassFunction(const std::vector<double>& PMF)
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

	DiscreteDist DiscreteDist::Add(const DiscreteDist& other) const
	{
		int64_t minResult = this->min + other.min;
		int64_t maxResult = this->Max() + other.Max();
		std::vector<double> PMFResult(static_cast<size_t>(maxResult - minResult + 1ll), 0.0);
		for (size_t i = 0; i < translatedPMF.size(); i++)
		{
			for (size_t j = 0; j < other.translatedPMF.size(); j++)
			{
				PMFResult[i + j] += this->translatedPMF[i] * other.translatedPMF[j];
			}
		}
		Trim(PMFResult, minResult);
		return DiscreteDist(PMFResult, minResult);
	}
	DiscreteDist DiscreteDist::TakeMaximumWith(int64_t value) const
	{
		int64_t max = Max();
		if (value <= min)
		{//simply copy. 
			return DiscreteDist(translatedPMF, min);
		}
		if (value >= max)
		{
			return GetConstantDist(value);
		}
		int64_t minResult = value;
		int64_t delta = value - min;

		std::vector<double> PMFResult((max - minResult) + 1ll, 0.0);
		for (int64_t i = 0; i <= delta; i++)
		{
			PMFResult[0] += translatedPMF[i];
		}
		

		for (int64_t i = delta + 1ll; i <= max - min; i++)
		{
			PMFResult[i - delta] = translatedPMF[i];
		}
		Trim(PMFResult, minResult);
		return DiscreteDist(PMFResult, minResult);
	}
	DiscreteDist DiscreteDist::Invert() const
	{
		int64_t minResult = -Max();
		int64_t l = Max() - min;
		int64_t range = l + 1ll;//equals translaterPMF.size() converted to int64_t
		std::vector<double> PMFResult(range);

		for (int64_t i = 0; i < range; i++)
		{
			PMFResult[i] = this->translatedPMF[l - i];
		}
		//No trimming as current PMF is already trimmed.
		return DiscreteDist(PMFResult, minResult);
	}

	
	int64_t DiscreteDist::Fractile(double alpha) const
	{
		double resProb = 1.0;
		double resProbTarget = 1 - alpha;
		int64_t i;
		int64_t max = Max();
		for (i = min; i <= max; i++)
		{
			resProb -= ProbabilityAt(i);
			if (resProb < resProbTarget)
			{
				break;
			}
		}
		return i;
	}



	DiscreteDist::DiscreteDist(std::vector<double>&& TranslatedProbMF, int64_t offSet)
		: translatedPMF(std::move(TranslatedProbMF)), min(offSet)
	{	
	}

	DiscreteDist::DiscreteDist(const std::vector<double>& TranslatedProbMF, int64_t offSet)
		: translatedPMF(TranslatedProbMF), min(offSet)
	{		
	}

	DiscreteDist DiscreteDist::GetConstantDist(int64_t constant)
	{
		//all mass on single value.
		std::vector<double> PMF = { 1.0 };
		return DiscreteDist(PMF, constant);
	}
	DiscreteDist DiscreteDist::GetZeroDist()
	{
		return GetConstantDist(0);
	}

	DiscreteDist DiscreteDist::GetCustomDist(const std::vector<double>& pmf, int64_t offset)
	{
		if (!IsProbMassFunction(pmf))
		{
			throw DynaPlex::Error("Input vector is not a probability mass function.");
		}
		return DiscreteDist(pmf, offset);
	}

	DiscreteDist DiscreteDist::GetCustomDist(std::vector<double>&& pmf, int64_t offset)
	{
		if (!IsProbMassFunction(pmf))
		{
			throw DynaPlex::Error("Input vector is not a probability mass function.");
		}
		return DiscreteDist(std::move(pmf), offset);
	}

	DiscreteDist DiscreteDist::GetPoissonDist(double mean)
	{
		if (mean < 0.0)
		{
			throw DynaPlex::Error("DiscreteDist: mean must be non-negative.");
		}
		std::vector<double> probVec;

		int64_t uBound = (int64_t)(mean + 7.0 + 7.0 * std::sqrt(mean));
		if (mean < 100.0)
		{
			probVec.reserve(uBound);
			int64_t i = 0;
			double factor = std::exp(-mean);
			while (i < uBound)
			{
				probVec.push_back(factor);
				factor *= mean / (++i);
			}
		}
		else
		{//use normal approximation:
			int64_t i = 0;
			while (i < uBound)
			{
				double next = 1.0 / std::sqrt(2 * PI * mean) *
					std::exp(-(i - mean) * (i - mean) / (2 * mean));
				probVec.push_back(next);
				i++;
			}
		}
		int64_t min = 0;
		Trim(probVec, min);
		return DiscreteDist(std::move(probVec), min);

	}

	DiscreteDist DiscreteDist::GetGeometricDist(double mean)
	{
		if (mean < 0.0)
		{
			throw DynaPlex::Error("DiscreteDist: mean must be non-negative.");
		}
		std::vector<double> probVec;
		double p = 1.0 / (1.0 + mean);
		double prob = p;
		while (prob > epsilon)
		{
			probVec.push_back(prob);
			prob *= (1 - p);
		}
		if (!IsProbMassFunction(probVec))
		{
			throw DynaPlex::Error("Input vector is not a probability mass function.");
		}
		return DiscreteDist(std::move(probVec), 0);

	}

	double DiscreteDist::ProbabilityAt(int64_t value) const 
	{
		if (value < min || value > Max())
		{
			return 0.0;
		}
		return translatedPMF[value - min];
	}

	double DiscreteDist::Expectation() const {
		double expectation = 0.0;
		for (const auto& [qty,prob] : *this) {
			expectation += qty * prob;
		}
		return expectation;
	}

	double DiscreteDist::Variance() const {
		double expectation = Expectation();
		double variance = 0.0;
		for (const auto& [qty, prob] : *this) {
			variance += prob * (qty - expectation) * (qty - expectation);
		}
		return variance;
	}

	double DiscreteDist::Entropy() const {
		double entropy = 0.0;
		for (const auto& [qty, prob] : *this) {
			if (prob > 0.0)
				entropy -= prob * std::log(prob);
		}
		return entropy;
	}

	

	double DiscreteDist::StandardDeviation() const {
		return std::sqrt(Variance());
	}



	int64_t DiscreteDist::GetSample(DynaPlex::RNG& rng) const {
		// Generate a uniform random number between 0 and 1
		double randomValue = rng.genUniform();
		double cumulativeProbability = 0.0;
		for (size_t i = 0; i < translatedPMF.size(); i++) {
			cumulativeProbability += translatedPMF[i];
			if (randomValue < cumulativeProbability) {
				return min + static_cast<int64_t>(i);
			}
		}

		// This should technically never be reached if probabilities sum to 1, 
		// but it handles potential numerical inaccuracies.
		return Max();
	}
	

}