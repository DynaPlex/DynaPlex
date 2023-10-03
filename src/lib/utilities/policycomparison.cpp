#include "dynaplex/policycomparison.h"
#include <sstream>
#include <cmath>

namespace DynaPlex {

    PolicyComparison PolicyComparison::GetComparison(const std::vector<double> vector)
    {
        std::vector<std::vector<double>> nested;
        nested.push_back(vector);
        return PolicyComparison(nested);
    }

    PolicyComparison::PolicyComparison(const std::vector<std::vector<double>>& nestedVector)
        : data(nestedVector) {

        if (nestedVector.size()==0)
        {
            throw Error("PolicyComparison: nestedVector must have non-zero length.");
        }

        // Check for uniformity of inner vector lengths and validity
        size_t len = nestedVector.front().size();
        for (const auto& vec : nestedVector) {
            if (vec.size() != len || vec.size() <= 1) {
                throw Error("PolicyComparison: All inner vectors must have equal length and length > 1.");
            }
        }

        // Compute means
        means.resize(nestedVector.size(), 0.0);
        for (size_t i = 0; i < nestedVector.size(); ++i) {
            for (const auto& value : nestedVector[i]) {
                means[i] += value;
            }
            means[i] /= len;
        }

        // Compute covariance matrix
        covariances.resize(nestedVector.size(), std::vector<double>(nestedVector.size(), 0.0));
        for (size_t i = 0; i < nestedVector.size(); ++i) {
            for (size_t j = 0; j < nestedVector.size(); ++j) {
                for (size_t k = 0; k < len; ++k) {
                    covariances[i][j] += (nestedVector[i][k] - means[i]) * (nestedVector[j][k] - means[j]);
                }
                covariances[i][j] /= (len - 1);
            }
        }
    }

   

    double PolicyComparison::mean(int64_t i, int64_t j) const {
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

            return means.at(i) - means.at(j);
        }
    }

    
    double PolicyComparison::standardError(int64_t i, int64_t j) const {
        size_t n = data.size();
        if (i >= n || i < 0)
            throw Error("PolicyComparison: index i out of range");
        if (j == -1 )
        {            
            return std::sqrt(covariances.at(i).at(i) / n);
        }
        else
        {
            if (j >= n || j < 0)
                throw Error("PolicyComparison: index j out of range");
            return std::sqrt((covariances.at(i).at(i) + covariances.at(j).at(j) - 2 * covariances.at(i).at(j)) / n);
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

}  // namespace DynaPlex
