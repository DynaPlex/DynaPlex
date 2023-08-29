#pragma once
#include <math.h>
#include <string>

namespace DynaPlex {

    class StatisticsKeeper
    {
    public:
        StatisticsKeeper();

        void AddStatistic(double value);

        int64_t numStatisticsGathered() const;

        double MuHat() const;

        double SigmaOfMean() const;

        double VarianceOfMean() const;

    private:
        int64_t count;
        double sumOfVal;
        double sumOfVal2;
    };

} // namespace DynaPlex
