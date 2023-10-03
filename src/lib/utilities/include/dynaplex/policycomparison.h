#pragma once

#include <vector>
#include <string>
#include "dynaplex/error.h"

namespace DynaPlex {
    /**
     * @class PolicyComparison
     * @brief This class is designed to compute and compare statistics of multiple policies.
     *
     * PolicyComparison computes the mean, variance, and covariance for a set of
     * policies, each represented as a vector of double values.
     */
    class PolicyComparison {
    private:
        std::vector<std::vector<double>> data;
        std::vector<double> means;
        std::vector<std::vector<double>> covariances;

    public:
        /**
         * @brief Construct a new PolicyComparison object.
         *
         * @param nestedVector A matrix-like structure where each row represents a policy dataset.
         */
        PolicyComparison(const std::vector<std::vector<double>>& nestedVector);

        static PolicyComparison GetComparison(const std::vector<double> vector);


        PolicyComparison(const std::vector<double>& vector);

        /**
         * @brief Get the mean difference between two policy datasets.
         *
         * @param i Index of the first policy dataset.
         * @param j Index of the second policy dataset. Defaults to -1, in which case the mean of i is returned.
         * @return The mean difference in return between the specified policy datasets.
         */
        double mean(int64_t i, int64_t j=-1) const;
        /**
         * @brief Get the standard error for the difference between two policy datasets.
         *
         * @param i Index of the first policy dataset.
         * @param j Index of the second policy dataset. Defaults to -1, in which case the standard error of i is returned.
         * @return The standard error for the difference between the specified policy datasets.
         */
        double standardError(int64_t i, int64_t j=-1) const;

        std::string ToString() const;
    };

}  // namespace DynaPlex
