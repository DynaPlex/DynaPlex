#include <iostream>
#include "dynaplex/error.h"
#include "dynaplex/statisticskeeper.h"
namespace DynaPlex {

    StatisticsKeeper::StatisticsKeeper() : count(0), sumOfVal(0.0), sumOfVal2(0.0) {}

    void StatisticsKeeper::AddStatistic(double value)
    {
        count++;
        sumOfVal += value;
        sumOfVal2 += value * value;
    }

    int64_t StatisticsKeeper::numStatisticsGathered() const
    {
        return count;
    }

    double StatisticsKeeper::MuHat() const
    {
        if (count == 0)
        {
            throw DynaPlex::Error("StatisticsKeeper: count is zero, no mean available.");
        }
        return sumOfVal / count;
    }

    double StatisticsKeeper::SigmaOfMean() const
    {
        if (count == 0)
        {
            throw DynaPlex::Error("StatisticsKeeper: count is zero, no standard deviation available.");
        }
        return sqrt(VarianceOfMean());
    }

    double StatisticsKeeper::VarianceOfMean() const
    {
        if (count <= 1)
        {
            throw DynaPlex::Error("StatisticsKeeper: count is too low, no variance available.");
        }
        double variance = (sumOfVal2 - (sumOfVal * sumOfVal / count)) / (count - 1);
        if (variance < 0.0)
        {
            return 0.0;
        }			
        return variance / count;
    }

} // namespace DynaPlex
