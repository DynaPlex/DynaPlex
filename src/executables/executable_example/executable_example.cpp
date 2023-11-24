#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/negative_binomial.hpp>

#include <iostream>

int main() {
    boost::math::binomial_distribution<> binom_dist(10, 0.5);

    for (int i = 0; i <= 10; ++i) {
        std::cout << "P(X = " << i << ") = " << boost::math::pdf(binom_dist, i) << std::endl;
    }

    boost::math::negative_binomial_distribution<> negbinom_dist(10, 0.5);
    
    for (int i = 0; i <= 10; ++i) {
        std::cout << "P(X = " << i << ") = " << boost::math::pdf(negbinom_dist, i) << std::endl;
    }


    return 0;
}