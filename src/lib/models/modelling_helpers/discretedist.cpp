#include "dynaplex/modelling_helpers/discretedist.h"
#include <math.h>

#include "dynaplex/error.h"

const double PI = std::atan(1.0) * 4;

bool DiscreteDist::IsProbMassFunction(std::vector<double> PMF)
{
    double TotalProb = 0.0;
    for(auto d : PMF)
    {
        TotalProb += d;
        if (d < 0.0)
        {
            return false;
        }
    }
    return std::abs(TotalProb - 1.0) < 1e-8;
}

DiscreteDist DiscreteDist::Add(const DiscreteDist& other)
{
    int64_t minResult = this->min + other.min;
    int64_t maxResult = this->max + other.max;
    std::vector<double> PMFResult(static_cast<size_t>(maxResult) - static_cast<size_t>(minResult) + static_cast<size_t>(1l), 0.0);
    for (size_t i = 0; i < TranslatedPMF.size(); i++)
    {

        for (size_t j = 0; j < other.TranslatedPMF.size(); j++)
        {
            PMFResult[i + j] += this->TranslatedPMF[i] * other.TranslatedPMF[j];
        }
    }
    return DiscreteDist(PMFResult, minResult);
}
DiscreteDist DiscreteDist::TakeMaximumWith(int64_t value)
{
    if (value <= min)
    {
        return DiscreteDist(TranslatedPMF, min);
    }
    if (value >= max)
    {
        return GetConstantDistribution(value);
    }
    int64_t minResult = value;
    int64_t delta = value - min;
    int64_t maxResult = this->max;

    std::vector<double> PMFResult((maxResult - minResult) + 1ll,0.0);
    for (int64_t i = 0; i <= delta; i++)
    {
        PMFResult[0] += TranslatedPMF[i];
    }
    for (size_t i = static_cast<size_t>(delta)+ static_cast<size_t>(1l); i <= static_cast<size_t>(max)- static_cast<size_t>(min) ; i++)
    {
        PMFResult[i - delta] = this->TranslatedPMF[i];
    }
    return DiscreteDist(PMFResult, static_cast<int64_t>(minResult));

}
DiscreteDist DiscreteDist::Invert()
{
    int64_t minResult = -this->max;
    int64_t l = this->max - this->min;
    std::vector<double> PMFResult(l + 1, 0.0);

    for (int64_t i = 0; i < l; i++)
    {
        PMFResult[i] = this->TranslatedPMF[l - i];
    }
    return DiscreteDist(PMFResult,minResult);
}

std::vector<double> DiscreteDist::ToPMF()
{
    if (min < 0)
    {
        throw DynaPlex::Error("Cannot represent PMF as std::vector: Distribution may take on negative values.");
    }
    std::vector<double> returnVal;
    returnVal.reserve(min+TranslatedPMF.size());
    for (int64_t i = 0; i < min; i++)
    {
        returnVal.push_back(0.0);
    }
    for (size_t i = 0; i < TranslatedPMF.size(); i++)
    {
        returnVal.push_back(TranslatedPMF[i]);
    }
    return returnVal;
}


int64_t DiscreteDist::GetFractile(double alpha)
{
    double resProb = 1.0;
    double resProbTarget = 1 - alpha;
    int64_t i;
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
DiscreteDist DiscreteDist::GetConstantDistribution(int64_t constant)
{
    return DiscreteDist({ 1.0 }, constant);
}
DiscreteDist DiscreteDist::GetZeroDistribution()
{
    return GetConstantDistribution(0);
}

std::vector<double> DiscreteDist::GetPoissonPMF(double rate)
{
    std::vector<double> returnVec;

    int64_t uBound = (int64_t)(rate + 7.0 + 7.0 * std::sqrt(rate));
    if (rate < 100.0)
    {
        returnVec.reserve(uBound);
        int64_t i = 0;
        double factor = std::exp(-rate);
        while (i < uBound)
        {
            returnVec.push_back(factor);
            factor *= rate / (++i);
        }
    }
    else
    {//use normal approximation:
        int64_t i = 0;
        while (i < uBound)
        {
            double next = 1.0 / std::sqrt(2 * PI * rate) *
                std::exp(-(i - rate) * (i - rate) / (2 * rate));
            returnVec.push_back(next);    
            i++;
        }
    }
    return returnVec;

}
/// <summary>
/// Zero-based geometric distribution
/// </summary>
std::vector<double> DiscreteDist::GetGeometricPMF(double mean)
{
    std::vector<double> returnVec;
    double p = 1.0 / (1.0 + mean);
    double prob = p;
    while (prob > 1e-16)
    {
        returnVec.push_back(prob);
        prob *= (1 - p);
    }
    return returnVec;

}

double DiscreteDist::ProbabilityAt(int64_t value)
{
    if (value < min || value > max)
	{
		return 0.0;
	}
	return TranslatedPMF[value - min];
}

double DiscreteDist::Expectation()
{
    double exp{ 0.0 };
    for (int64_t i = min; i <= max; i++)
    {
        double prob = ProbabilityAt(i);
        exp += prob * i;
    }
    return exp;
}

DiscreteDist::DiscreteDist(const std::vector<double>& TranslatedProbMF, int64_t offSet)
{
	if (!IsProbMassFunction(TranslatedProbMF))
	{
        throw DynaPlex::Error("DiscretDist:: Probabilities should be nonnegative and sum to 1.0");
	}
    size_t iter = 0;
    while (TranslatedProbMF[iter] <= epsilon)
    {
        iter++;
    }
    size_t minPos = iter;
    iter = TranslatedProbMF.size() - 1;
    while (TranslatedProbMF[iter] < epsilon)
    {
        iter--;
    }
    size_t maxPos = iter;
    this->TranslatedPMF.reserve(maxPos + 1 - minPos);
    for (size_t i = minPos; i <= maxPos; i++)
    {
        this->TranslatedPMF.push_back(TranslatedProbMF[i]);
    }
    min = offSet + static_cast<int64_t>(minPos);
    max = min + static_cast<int64_t>(this->TranslatedPMF.size()) - 1l;
}
