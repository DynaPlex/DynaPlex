#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/error.h"
#include <algorithm>  //sort
#include <cmath>  //atan
#include <unordered_map>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/negative_binomial.hpp>
#include <boost/math/distributions/geometric.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <random>


namespace DynaPlex
{

	enum class DistType {
		CONSTANT,
		ZERO,
		POISSON,
		GEOMETRIC,
		CUSTOM,
		BINOMIAL,
		NEGATIVE_BINOMIAL,
		ADAN_EENIGE_RESING,
		UNKNOWN
	};


	int64_t DiscreteDist::DistinctValueCount() const {
		return static_cast<int64_t>(translatedPMF.size());
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
			{"custom", DistType::CUSTOM},
			{"binomial", DistType::BINOMIAL},
			{"negative_binomial", DistType::NEGATIVE_BINOMIAL},
			{"adan_eenige_resing", DistType::ADAN_EENIGE_RESING}
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

		case DistType::ADAN_EENIGE_RESING:
		{
			double mean, stdev;
			vars.Get("mean", mean);
			vars.Get("stdev", stdev);
			*this = GetAdanEenigeResingDist(mean, stdev);
			break;
		}

		case DistType::BINOMIAL:
		{
			double n;
			double p;
			vars.Get("n", n);
			vars.Get("p", p);
			*this = GetBinomialDist(n, p);
			break;
		}

		case DistType::NEGATIVE_BINOMIAL:
		{
			double r;
			double p;
			vars.Get("r", r);
			vars.Get("p", p);
			*this = GetNegativeBinomialDist(r, p);
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
		double epsilon_used = epsilon;
		size_t iter = 0;
		while (ToBeTrimmed[iter] <= epsilon_used)
		{
			iter++;
		}
		size_t minPos = iter;

		iter = ToBeTrimmed.size() - 1;
		while (ToBeTrimmed[iter] < epsilon_used)
		{
			iter--;
		}
		size_t maxPos = iter;

		if (minPos != 0 || maxPos != ToBeTrimmed.size())
		{
			std::vector<double> TrimmedPMF{};
			TrimmedPMF.reserve(maxPos + 1 - minPos);

			for (size_t i = minPos; i <= maxPos; i++)
			{
				TrimmedPMF.push_back(ToBeTrimmed[i]);
			}
			min += static_cast<int64_t>(minPos);
			ToBeTrimmed = std::move(TrimmedPMF);
		}
	}


	bool CheckAdanEenigeResingCondition(double mean, double stdev)
	{
		// Checking lemma 2.1 (necessary condition for non-negative discrete distribution)
		// Adan, Eenige & Resing (1995)
		const double epsilon = 1e-8; // Tolerance level
		bool nearInteger = std::abs(mean - std::round(mean)) < epsilon;
		bool nearZeroSigma = std::abs(stdev) < epsilon;
		if (mean <= 0.0)
			throw DynaPlex::Error("DiscreteDist: AdanEenigeResing - mean should be strictly positive");
		if (stdev < 0.0)
			throw DynaPlex::Error("DiscreteDist: AdanEenigeResing - sigma should be non-negative");
		double delta = mean - std::floor(mean);
		bool condition = stdev * stdev > (delta) * (1.0 - delta);
		return (condition || (nearInteger && nearZeroSigma));
	}

	double DiscreteDist::LeastVarianceRequiredForAERFit(double mu)
	{
		double delta = mu - std::floor(mu);
		return delta * (1.0 - delta);
	}

	DiscreteDist DiscreteDist::GetGeometricDist(double mean)
	{
		if (mean < 0.0)
		{
			throw DynaPlex::Error("DiscreteDist: mean must be non-negative.");
		}
		double p = 1.0 / (1.0 + mean);
		return GetGeometricDistFromProb(p);

	}
	DiscreteDist DiscreteDist::GetGeometricDistFromProb(double p)
	{
		if (p > 1.0 || p < 0.0)
			throw DynaPlex::Error("DiscreteDist: GetGeometricDistFromProb must be in [0.0,1.0).");
		std::vector<double> probVec;
		int64_t max = 3 + boost::math::geometric_distribution<double>::find_maximum_number_of_trials(0, 1.0 - p, 1.0 - epsilon);
		probVec.reserve(max);
		double mult = 1.0 - p;
		double prob = p;
		while (prob > epsilon)
		{
			probVec.push_back(prob);
			prob *= mult;
		}
		if (probVec.size() > max)
			throw DynaPlex::Error("GetGeometricDistFromProb - Invalid maximum: p=" + std::to_string(p) + " computed max=" + std::to_string(max) + " actual=" + std::to_string(probVec.size()));
		if (!IsProbMassFunction(probVec))
		{
			throw DynaPlex::Error("Input vector is not a probability mass function.");
		}
		return DiscreteDist(std::move(probVec), 0);
	}

	int64_t DiscreteDist::GetGeometricSample(double mean, DynaPlex::RNG& rng)
	{
		if (mean < 0.0)
		{
			throw DynaPlex::Error("DiscreteDist: mean must be non-negative.");
		}
		double p = 1.0 / (1.0 + mean);
		return GetGeometricSampleFromProb(p, rng);
	}

	int64_t DiscreteDist::GetGeometricSampleFromProb(double p, DynaPlex::RNG& rng)
	{
		if (p > 1.0 || p < 0.0)
			throw DynaPlex::Error("DiscreteDist: GetGeometricDistFromProb must be in [0.0,1.0).");

		std::geometric_distribution<> dist(p);
		return dist(rng.gen());
	}
	int64_t DiscreteDist::GetAdanEenigeResingSample(double mean, double stdev, DynaPlex::RNG& rng)
	{
		// Check if it is even a feasible distribution, satisfying Adan's demand condition
		if (!CheckAdanEenigeResingCondition(mean, stdev))
			throw DynaPlex::Error("DiscreteDist: Condition of Adan, van Eenige and Resing not satisied.");

		double a = (stdev / mean) * (stdev / mean) - 1 / mean;
		if (std::abs(a) < 1e-6) // Poisson distribution
		{
			return GetPoissonSample(mean, rng);
		}
		else if (a < 0) // Binomial mixture distribution
		{
			if (std::abs(a + 1) < 1e-10)
			{
				if (a > -1)
					a = -1 + 1e-10;
				else
					a = -1 - 1e-10;
			}
			int64_t k = static_cast<int64_t>(std::floor(1.0 / -a));
			double q = (1.0 + a * (1.0 + k) + std::sqrt(-a * k * (1.0 + k) - k)) / (1.0 + a);
			double p = mean / (k + 1.0 - q);
			if (rng.genUniform() < q)
				return DiscreteDist::GetBinomialSample(k, p, rng);
			else
				return DiscreteDist::GetBinomialSample(k + 1, p, rng);
		}
		else if (a < 1) // Negative binomial mixture
		{
			int64_t k = static_cast<int64_t>(std::floor(1.0 / a));
			double q = ((1.0 + k) * a - std::sqrt((1.0 + k) * (1.0 - a * k))) / (1.0 + a);
			double p = mean / (k + 1.0 - q + mean);

			if (rng.genUniform() < q)
				return DiscreteDist::GetNegativeBinomialSample(k, 1 - p, rng);
			else
				return DiscreteDist::GetNegativeBinomialSample(k + 1, 1 - p, rng);
		}
		else // Geometric mixture: a>1
		{
			double var_positive = 1.0 + a + std::sqrt(a * a - 1.0);
			double var_negative = 1.0 + a - std::sqrt(a * a - 1.0);
			double p1 = (mean * var_positive) / (2.0 + mean * var_positive);
			double q1 = 1.0 / var_positive;
			double p2 = (mean * var_negative) / (2.0 + mean * var_negative);
			if (rng.genUniform() < q1)
				return DiscreteDist::GetGeometricSampleFromProb(1.0 - p1, rng);
			else
				return DiscreteDist::GetGeometricSampleFromProb(1.0 - p2, rng);
		}
	}

	DiscreteDist DiscreteDist::GetAdanEenigeResingDist(double mean, double stdev)
	{
		// Check if it is even a feasible distribution, satisfying Adan's demand condition
		if (!CheckAdanEenigeResingCondition(mean, stdev))
			throw DynaPlex::Error("DiscreteDist: Condition of Adan, van Eenige and Resing not satisied.");

		double a = (stdev / mean) * (stdev / mean) - 1 / mean;
		if (std::abs(a) < 1e-6) // Poisson distribution
		{
			return DiscreteDist::GetPoissonDist(mean);
		}
		else if (a < 0) // Binomial mixture distribution
		{
			if (std::abs(a + 1) < 1e-10)
			{
				if (a > -1)
					a = -1 + 1e-10;
				else
					a = -1 - 1e-10;
			}
			int64_t k = static_cast<int64_t>(std::floor(1.0 / -a));
			double q = (1.0 + a * (1.0 + k) + std::sqrt(-a * k * (1.0 + k) - k)) / (1.0 + a);
			double p = mean / (k + 1.0 - q);
			auto pmf1 = DiscreteDist::GetBinomialDist(k, p);
			auto pmf2 = DiscreteDist::GetBinomialDist((k + 1), p);
			return pmf2.Mix(pmf1, q);
		}
		else if (a < 1) // Negative binomial mixture
		{
			int64_t k = static_cast<int64_t>(std::floor(1.0 / a));
			double q = ((1.0 + k) * a - std::sqrt((1.0 + k) * (1.0 - a * k))) / (1.0 + a);
			double p = mean / (k + 1.0 - q + mean);
			auto pmf1 = DiscreteDist::GetNegativeBinomialDist(k, 1 - p);
			auto pmf2 = DiscreteDist::GetNegativeBinomialDist((k + 1), 1 - p);
			return pmf2.Mix(pmf1, q);
		}
		else // Geometric mixture: a>1
		{
			double var_positive = 1.0 + a + std::sqrt(a * a - 1.0);
			double var_negative = 1.0 + a - std::sqrt(a * a - 1.0);
			double p1 = (mean * var_positive) / (2.0 + mean * var_positive);
			double q1 = 1.0 / var_positive;
			double p2 = (mean * var_negative) / (2.0 + mean * var_negative);
			auto pmf1 = DiscreteDist::GetGeometricDistFromProb(1.0 - p1);
			auto pmf2 = DiscreteDist::GetGeometricDistFromProb(1.0 - p2);
			return pmf2.Mix(pmf1, q1);
		}
	}

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

	int64_t DiscreteDist::GetBinomialSample(int64_t n, double p, DynaPlex::RNG& rng)
	{
		if (p < 0.0 || p>1.0)
		{
			throw DynaPlex::Error("DiscreteDist: p must be in [0,1].");
		}
		if (n < 0)
			throw DynaPlex::Error("DiscreteDist: n must be nonnegative.");

		std::binomial_distribution<> dist(n, p);
		return dist(rng.gen());
	}

	int64_t DiscreteDist::GetNegativeBinomialSample(int64_t r, double p, DynaPlex::RNG& rng)
	{
		if (p < 0.0 || p>1.0)
		{
			throw DynaPlex::Error("DiscreteDist: p must be in [0,1].");
		}
		if (r < 0)
			throw DynaPlex::Error("DiscreteDist: r must be nonnegative.");

		std::negative_binomial_distribution<> dist(r, p);
		/*	boost::math::negative_binomial_distribution<> bdist(static_cast<double>(r), p);
			int64_t i = 0;
			double prob = boost::math::pdf(bdist, i);
			double prob_left = rng.genUniform();
			while (prob_left > prob)
			{
				prob_left -= prob;
				prob = boost::math::pdf(bdist, ++i);

			}
			return i;*/


		return dist(rng.gen());
	}

	int64_t DiscreteDist::GetPoissonSample(double mean, DynaPlex::RNG& rng)
	{
		if (mean < 0.0)
		{
			throw DynaPlex::Error("DiscreteDist: mean must be non-negative.");
		}
		std::poisson_distribution<> dist(mean);
		return dist(rng.gen());
	}


	DiscreteDist DiscreteDist::GetPoissonDist(double mean)
	{
		if (mean < 0.0)
		{
			throw DynaPlex::Error("DiscreteDist: mean must be non-negative.");
		}
		std::vector<double> probVec;
		int64_t uBound = (int64_t)(mean + 7.0 + 7.0 * std::sqrt(mean));
		probVec.reserve(uBound);
		if (mean < 100.0)
		{
			int64_t i = 0;
			double factor = std::exp(-mean);
			while (i < uBound)
			{
				probVec.push_back(factor);
				factor *= mean / (++i);
			}
		}
		else
		{
			boost::math::poisson_distribution<> pois_dist(mean);
			for (int64_t i = 0; i < uBound; i++)
			{
				probVec.push_back(boost::math::pdf(pois_dist, i));
			}
		}
		int64_t min = 0;
		Trim(probVec, min);
		return DiscreteDist(std::move(probVec), min);
	}


	DiscreteDist DiscreteDist::GetBinomialDist(double n, double p) {
		if (n < 0 || p < 0.0 || p > 1.0) {
			throw DynaPlex::Error("Invalid parameters for binomial distribution.");
		}

		boost::math::binomial_distribution<> binomDist(n, p);
		std::vector<double> probs;
		probs.reserve(n + 1);
		for (int64_t i = 0; i <= n; ++i) {
			probs.push_back(boost::math::pdf(binomDist, i));
		}
		int64_t min = 0;
		Trim(probs, min);
		return DiscreteDist::GetCustomDist(probs, min);
	}

	DiscreteDist DiscreteDist::GetNegativeBinomialDist(double r, double p) {
		if (r <= 0 || p <= 0.0 || p > 1.0) {
			throw DynaPlex::Error("Invalid parameters for negative binomial distribution.");
		}
		boost::math::negative_binomial_distribution<> negBinomDist(r, p);
		std::vector<double> probs;
		int64_t cutoff = boost::math::quantile(boost::math::complement(negBinomDist, epsilon));
		probs.reserve(cutoff + 1);
		for (int64_t i = 0; i <= cutoff; ++i) {
			probs.push_back(boost::math::pdf(negBinomDist, i));
		}
		int64_t min = 0;
		Trim(probs, min);
		return DiscreteDist::GetCustomDist(probs, min);
	}

	DiscreteDist DiscreteDist::Mix(const DiscreteDist& other, double prob_of_other) const {
		// Check if the mixing probability is valid
		if (prob_of_other < 0.0 || prob_of_other > 1.0) {
			throw DynaPlex::Error("DiscreteDist: Mix probability must be between 0.0 and 1.0.");
		}

		// Determine the range of the mixed distribution
		int64_t mixedMin = std::min(this->min, other.min);
		int64_t mixedMax = std::max(this->Max(), other.Max());

		// Initialize the PMF for the mixed distribution
		std::vector<double> mixedPMF(static_cast<size_t>(mixedMax - mixedMin + 1), 0.0);

		// Calculate the mixed PMF
		for (int64_t i = mixedMin; i <= mixedMax; ++i) {
			double thisProb = this->ProbabilityAt(i);
			double otherProb = other.ProbabilityAt(i);
			mixedPMF[i - mixedMin] = (1.0 - prob_of_other) * thisProb + prob_of_other * otherProb;
		}

		// Ensure the new PMF is still a valid probability mass function
		if (!IsProbMassFunction(mixedPMF)) {
			throw DynaPlex::Error("Mixed PMF does not sum to 1.0 or has negative probabilities.");
		}

		// Create a new distribution with the mixed PMF
		return DiscreteDist(mixedPMF, mixedMin);
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
		for (const auto& [qty, prob] : *this) {
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