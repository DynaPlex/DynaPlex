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

    void PolicyComparison::Initialize() {
        if (data.size() == 0) {
            throw DynaPlex::Error("PolicyComparison: nestedVector must have non-zero length.");
        }

        // Check for uniformity of inner vector lengths and validity
        size_t len = data.front().size();
        for (const auto& vec : data) {
            if (vec.size() != len) {
                throw DynaPlex::Error("PolicyComparison: All inner vectors must have equal length and length > 1.");
            }
        }

        

        // Compute means
        means.resize(data.size(), 0.0);
        for (size_t i = 0; i < data.size(); ++i) {
            for (const auto& value : data[i]) {
                means[i] += value;
            }
            means[i] /= len;
        }

        // Compute covariance matrix
        covariances.resize(data.size(), std::vector<double>(data.size(), 0.0));
        for (size_t i = 0; i < data.size(); ++i) {
            for (size_t j = 0; j < data.size(); ++j) {
                for (size_t k = 0; k < len; ++k) {
                    covariances[i][j] += (data[i][k] - means[i]) * (data[j][k] - means[j]);
                }
                covariances[i][j] /= (len - 1);
            }
        }
    }

    PolicyComparison::PolicyComparison(const std::vector<std::vector<double>>& nestedVector)
        : data(nestedVector) {
        Initialize();        
    }
    PolicyComparison::PolicyComparison(std::vector<std::vector<double>>&& nestedVector)
        : data(std::move(nestedVector)) {
        Initialize();
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
        size_t len = data.front().size();
        if (len == 1) {
            throw Error("PolicyComparison: cannot compute standardError since there is only one datapoint per alternative. ");
        }
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
