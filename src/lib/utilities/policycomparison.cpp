#include "dynaplex/policycomparison.h"
#include <sstream>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iostream>

namespace DynaPlex {

    PolicyComparison PolicyComparison::GetComparison(const std::vector<double> vector)
    {
        std::vector<std::vector<double>> nested;
        nested.push_back(vector);
        return PolicyComparison(nested);
    }

    void PolicyComparison::Initialize() {
        size_t n = data.size();
        if (n == 0) {
            throw DynaPlex::Error("PolicyComparison: nestedVector must have non-zero length.");
        }

        // Check for uniformity of inner vector lengths and validity
        size_t len = data.front().size();
        for (const auto& vec : data) {
            if (vec.size() != len) {
                isRectangular = false;
                break;
            }
        }

        // Initialize mean and covariance matrices
        means.resize(data.size(), 0.0);
        covariances.resize(data.size(), std::vector<double>(data.size(), 0.0));

        if (isRectangular) {
            // Compute means
            for (size_t i = 0; i < n; ++i) {
                for (const auto& value : data[i]) {
                    means[i] += value;
                }
                means[i] /= len;
            }

            // Compute covariance matrix
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = 0; j < n; ++j) {
                        for (size_t k = 0; k < len; ++k) {
                            covariances[i][j] += (data[i][k] - means[i]) * (data[j][k] - means[j]);
                        }
                    covariances[i][j] /= (len - 1);
                }
            }
        }
        else {
            // Compute means
            for (size_t i = 0; i < n; ++i) {
                for (const auto& value : data[i]) {
                    means[i] += value;
                }
                means[i] /= data[i].size();
            }

            // Compute covariance matrix, can only compute the diagonals because of the varying inner vector lengths
            for (size_t i = 0; i < n; ++i) {
                    for (size_t k = 0; k < data[i].size(); ++k) {
                        covariances[i][i] += (data[i][k] - means[i]) * (data[i][k] - means[i]);
                    }
                covariances[i][i] /= (data[i].size() - 1);              
            }
        }
    }

    PolicyComparison::PolicyComparison(const std::vector<std::vector<double>>& nestedVector)
        : data(nestedVector){
        Initialize();
    }
    PolicyComparison::PolicyComparison(std::vector<std::vector<double>>&& nestedVector)
        : data(std::move(nestedVector)){
        Initialize();
    }
   

    double PolicyComparison::mean(int64_t i, int64_t j, bool pairedSamples) const {
        size_t n = data.size();
        if (i >= n || i < 0)
            throw Error("PolicyComparison: index i out of range");

        if (j == -1)
        {
            return means.at(i);
        }
        else
        {
            if (j >= n || j < 0)
                throw Error("PolicyComparison: index j out of range");

            if ((isRectangular) || (!isRectangular && !pairedSamples)) {
                return means.at(i) - means.at(j);
            }
            else {
                size_t min_number_of_observation = std::min(data[i].size(), data[j].size());
                double mean_diff{ 0.0 };
                for (size_t k = 0; k < min_number_of_observation; ++k) {
                    mean_diff += data[i][k] - data[j][k];
                }
                mean_diff /= min_number_of_observation;
                return mean_diff;
            }
        }
    }
    
    double PolicyComparison::standardError(int64_t i, int64_t j, bool pairedSamples) const {
        size_t n = data.size();
        if (i >= n || i < 0)
            throw Error("PolicyComparison: index i out of range");

        if (isRectangular) {
            size_t len = data.front().size();
            if (len == 1) {
                throw Error("PolicyComparison: cannot compute standardError since there is only one datapoint per alternative. ");
            }

            if (j == -1)
            {
                return std::sqrt(covariances.at(i).at(i) / len);
            }
            else
            {
                if (j >= n || j < 0)
                    throw Error("PolicyComparison: index j out of range");
                return std::sqrt((covariances.at(i).at(i) + covariances.at(j).at(j) - 2 * covariances.at(i).at(j)) / len);
            }
        }
        else {
            if (j == -1) {
                if (data[i].size() == 1) {
                    throw Error("PolicyComparison: cannot compute standardError since there is only one datapoint per alternative.");
                }
                return std::sqrt(covariances.at(i).at(i) / data[i].size());
            }
            else {
                if (j >= n || j < 0)
                    throw Error("PolicyComparison: index j out of range");
                if (data[i].size() == 1 || data[j].size() == 1) {
                    throw Error("PolicyComparison: cannot compute standardError since there is only one datapoint per alternative.");
                }

                // return the weighted standard deviation
                if (!pairedSamples) {
                    return std::sqrt((covariances.at(i).at(i) / data[i].size() + covariances.at(j).at(j) / data[j].size()));
                }
                else {// return the standard deviation obtained by paired values
                    size_t min_number_of_observation = std::min(data[i].size(), data[j].size());
                    double mean_i{ 0.0 };
                    double mean_j{ 0.0 };
                    for (size_t k = 0; k < min_number_of_observation; ++k) {
                        mean_i += data[i][k];
                        mean_j += data[j][k];
                    }
                    mean_i /= min_number_of_observation;
                    mean_j /= min_number_of_observation;

                    double covariance_i{ 0.0 };
                    double covariance_j{ 0.0 };
                    double covariance_ij{ 0.0 };
                    for (size_t k = 0; k < min_number_of_observation; ++k) {
                        covariance_i += (data[i][k] - mean_i) * (data[i][k] - mean_i);
                        covariance_j += (data[j][k] - mean_j) * (data[j][k] - mean_j);
                        covariance_ij += (data[i][k] - mean_i) * (data[j][k] - mean_j);
                    }
                    covariance_i /= (min_number_of_observation - 1);
                    covariance_j /= (min_number_of_observation - 1);
                    covariance_ij /= (min_number_of_observation - 1);

                    return std::sqrt((covariance_i + covariance_j - 2 * covariance_ij) / min_number_of_observation);
                }
            }
        }
    }

    std::string PolicyComparison::ToString() const {
        // For simplicity, just returning means for now.
        std::string result = "Means:\n";
        for (const auto& m : means) {
            result += std::to_string(m) + " ";
        }
        return result;
    }

    void PolicyComparison::ComputeProbabilities(bool ValueBased) {
        size_t n = data.size();
        probs.resize(n, 0.0);
        
        // discard alternatives with small inner-vector lengths - worse than the others
        std::vector<bool> masked(n, true);
        if (maskAlternatives)
        {
            double percentage = 0.5;
            // Handle the special case of invalid percentage.
            if (percentage < 0.0 || percentage > 1.0) {
                throw Error("PolicyComparison: the percentage of masking before probability calculation should be between 0 and 1.");
            }

            // Calculate the actual number of true values needed based on the percentage.
            size_t numKeep = std::max(static_cast<size_t>(ceil(n * percentage)), (size_t)1);
            masked = mask(numKeep);
        }

        if (ValueBased) {
            double temperature = 0.5; // Adjust the temperature as needed, do not set it too low.
            if (temperature <= 0) {
                throw Error("PolicyComparison: temperature value for softmax must be positive");
            }
            if (z_statistics.empty()) {
                throw Error("PolicyComparison: z-statistics are missing when calculating probabilities. Try calling ComputeZstatistics() beforehand.");
            }

            std::vector<double> exp_values;
            exp_values.resize(n, 0.0);
            double sum_exp_values{ 0.0 };
            // Compute the exponentiated values adjusted for the temperature
            for (size_t i = 0; i < n; i++) {
                if (masked[i]) {
                    double exp_value = exp(-z_statistics.at(i) / temperature);
                    exp_values.at(i) = exp_value;
                    sum_exp_values += exp_value;
                }
            }
            // Divide the exponentiated values by the sum to get probabilities
            for (size_t i = 0; i < n; i++) {
                probs.at(i) = (exp_values.at(i) / sum_exp_values);
            }
        }
        else {
            size_t len{ 0 };
            if (isRectangular) {
                len = data.front().size();
            }
            else {
                for (size_t i = 0; i < n; i++) {
                    if (data[i].size() > len) {
                        len = data[i].size();
                    }
                }
            }
            if (len == 0) {
                throw Error("PolicyComparison: cannot compute probabilities since there is no datapoint per alternative. ");
            }

            for (size_t j = 0; j < len; j++) {
                double best_score = -std::numeric_limits<double>::infinity();
                size_t best_alternative{ 0 };
                for (size_t i = 0; i < n; i++) {
                    if (data[i].size() >= j+1 && masked[i]) {
                        double score = data.at(i).at(j);
                        if (score > best_score) {
                            best_score = score;
                            best_alternative = i;
                        }
                    }
                }
                probs.at(best_alternative) += 1.0;
            }
            for (size_t i = 0; i < n; i++) {
                probs.at(i) /= len; // note that this division will create negative bias for alternatives with missing observations if any
            }
        }
    }

    double PolicyComparison::GetProbability(int64_t i) const {
        size_t n = data.size();
        if (i >= n || i < 0)
            throw Error("PolicyComparison: index i out of range");
        if (probs.empty()) {
            throw Error("PolicyComparison: probabilities are missing. Call ComputeProbabilities() beforehand.");
        }

        return probs.at(i);
    }

    void PolicyComparison::ComputeZstatistics(int64_t i) {
        size_t n = data.size();
        if (i >= n || i < 0)
            throw Error("PolicyComparison: index i out of range");

        z_statistics.resize(n, 0.0);

        if (isRectangular) {
            size_t len = data.front().size();
            if (len == 1) {
                throw Error("PolicyComparison: cannot compute z-statistics since there is only one datapoint per alternative.");
            }

            for (size_t j = 0; j < n; j++) {
                double zValue{ 5.0 };  // do not set it too high for stability in case of softmax calculations
                if (i != j) {
                    double mu = mean(i, j);
                    double sigma = standardError(i, j);
                    if (sigma > 0.0)
                    {
                        zValue = mu / sigma;
                    }
                    z_statistics.at(j) = zValue;
                }
                else {
                    z_statistics.at(j) = 0.0;
                }
            }
        }
        else {
            for (size_t j = 0; j < n; j++) {
                if (data[i].size() == 1 || data[j].size() == 1) {
                    throw Error("PolicyComparison: cannot compute z-statistics since there is only one datapoint per an alternative.");
                }

                if (i != j) {
                    // first calculate z-value from Welch's t-test
                    double mu = mean(i, j, false);
                    double sigma = standardError(i, j, false);
                    double zValue = 5.0; // do not set it too high for stability in case of softmax calculations
                    if (sigma > 0.0)
                    {
                        zValue = mu / sigma;
                    }

                    // now calculate according to paired values
                    double mu_paired = mean(i, j, true);
                    double sigma_paired = standardError(i, j, true);
                    double zValue_paired = 5.0; // do not set it too high for stability in case of softmax calculations
                    if (sigma_paired > 0.0)
                    {
                        zValue_paired = mu_paired / sigma_paired;
                    }
                    z_statistics.at(j) = std::max(zValue, zValue_paired);
                }
                else {
                    z_statistics.at(j) = 0.0;
                }
            }
        }
    }

    double PolicyComparison::GetZstatistic(int64_t i) const {
        size_t n = data.size();
        if (i >= n || i < 0)
            throw Error("PolicyComparison: index i out of range");
        if (z_statistics.empty()) {
            throw Error("PolicyComparison: z-statistics are missing. Call ComputeZstatistics() beforehand.");
        }

        return z_statistics.at(i);
    }

    std::vector<bool> PolicyComparison::mask(size_t numKeep) {
        size_t n = data.size();
        std::vector<size_t> sizes;
        std::vector<double> values;
        sizes.reserve(n);
        values.reserve(n);
        for (int64_t i = 0; i < n; i++)
        {
            sizes.push_back(data[i].size());
            values.push_back(mean(i));
        }

        // Create a vector of indices based on the sorting of the values.
        std::vector<size_t> indices(n);
        std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., n-1
        // Sort the indices based on the frequencies, and in case of a tie, use the mean values
        std::sort(indices.begin(), indices.end(),
            [&sizes, &values](size_t i1, size_t i2) {
                if (sizes[i1] == sizes[i2])
                    return values[i1] > values[i2]; // Tiebreaker with values
                return sizes[i1] > sizes[i2]; // Primary sorting criteria
            }
        );

        // Initialize the boolean vector with false.
        std::vector<bool> topPercentageTrue(n, false);
        // Assign true to the top percentage of indices.
        for (size_t i = 0; i < numKeep; ++i) {
            topPercentageTrue[indices[i]] = true;
        }

        return topPercentageTrue;
    }

}  // namespace DynaPlex
