#pragma once
#include <vector>
#include <string>
#include "dynaplex/error.h"

namespace DynaPlex {
    /**
     * @class PolicyComparison
     * @brief This class is designed to compute and compare statistics of multiple policies or other options. 
     *
     * PolicyComparison computes the mean, variance, and covariance for a set of
     * policies, each represented as a vector of double values.
     */
    class PolicyComparison {
    private:
        std::vector<std::vector<double>> data;
        std::vector<double> means;
        std::vector<std::vector<double>> covariances;
        std::vector<double> probs;
        std::vector<double> z_statistics;
        bool isRectangular = true;
        bool maskAlternatives = true;

        void Initialize();

        std::vector<bool> mask(size_t numKeep);

    public:
        /**
         * @brief Construct a new PolicyComparison object.
         *
         * @param nestedVector A matrix-like structure where each row represents a policy dataset.
         */
        PolicyComparison(const std::vector<std::vector<double>>& nestedVector);


        /**
        * @brief Construct a new PolicyComparison object.
        *
        * @param nestedVector A matrix-like structure where each row represents a policy dataset.
        */
        PolicyComparison(std::vector<std::vector<double>>&& nestedVector);

        static PolicyComparison GetComparison(const std::vector<double> vector);


        PolicyComparison(const std::vector<double>& vector);

        /**
         * @brief Get the mean difference between two policy datasets.
         *
         * @param i Index of the first policy dataset.
         * @param j Index of the second policy dataset. Defaults to -1, in which case the mean of i is returned.
         * @param pairedSamples Condition for calculating means either on the whole sample for the each policy or on the paired samples.
         * @return The mean difference in return between the specified policy datasets.
         */
        double mean(int64_t i, int64_t j=-1, bool pairedSamples=false) const;
        /**
         * @brief Get the standard error for the difference between two policy datasets.
         *
         * @param i Index of the first policy dataset.
         * @param j Index of the second policy dataset. Defaults to -1, in which case the standard error of i is returned.
         * @param pairedSamples Condition for calculating either weighted standard deviation (default) or paired one if a second policy is given.
         * @return The standard error for the difference between the specified policy datasets.
         */
        double standardError(int64_t i, int64_t j=-1, bool pairedSamples=false) const;

        std::string ToString() const;

        /**
         * @brief Compute probability of being the best for each alternative (policy).
         * 
         * @param ValueBased Condition when calculating probabilities. If true (default), based on the values; if false, based on ranking. 
        */
        void ComputeProbabilities(bool ValueBased=true);
        /**
         * @brief Returns the probability of an alternative (policy) to be the best one.
         *
         * @param i Index of the alternative (policy).
         */
        double GetProbability(int64_t i) const;
        /**
         * @brief Computes the z-statistics between each alternative (policy) and the given one.
         * 
         * @param i Index of the alternative (policy).
         */
        void ComputeZstatistics(int64_t i);
        /**
         * @brief Returns the computed z-statistic for a given alternative (policy).
         *
         * @param i Index of the alternative (policy).
         */
        double GetZstatistic(int64_t i) const;

    };

}  // namespace DynaPlex
