#pragma once
#include <vector>
#include <stdlib.h>
#include <string>
#include <iostream>

class DiscreteDist
{
	std::vector<double> TranslatedPMF{};
	int64_t min{ 0 };
	int64_t max{ 0 };

	const double epsilon{ 1e-16 };


	static DiscreteDist GetConstantDistribution(int64_t constant);
	static DiscreteDist GetZeroDistribution();

	static std::vector<double>  GetPoissonPMF(double rate);
	static std::vector<double>  GetGeometricPMF(double mean);


	bool IsProbMassFunction(std::vector<double> PMF);


public:
	int64_t Max()
	{
		return max;
	}
	int64_t Min()
	{
		return min;
	}

	std::vector<double> ToPMF();


	int64_t GetFractile(double alpha);

	DiscreteDist Add(const DiscreteDist& other);

	DiscreteDist TakeMaximumWith(int64_t value);

	DiscreteDist Invert();

	double ProbabilityAt(int64_t value);

	double Expectation();

	DiscreteDist(const std::vector<double>& TranslatedPMF, int64_t offSet = 0ll);

};

