#include <iostream>
#include <deque>
#include <gtest/gtest.h>
#include <vector>
#include <iterator>
#include "dynaplex/error.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/jointdiscretedist.h"
#include "dynaplex/rng.h"

namespace DynaPlex::Tests {

	TEST(jointdiscretedist, joint_dist_tests) {
		// Construct customs distribution
		std::vector<double> probs = { 0.1, 0.5, 0.3, 0.1 }; //With offset -1, these will be probabilities for -1, 0, 1, 2 respectively
		int64_t min = -1;
		DynaPlex::VarGroup vg;
		vg.Add("type", "custom");
		vg.Add("probs", probs);
		vg.Add("offset", min); // Setting the offset so that the first probability is -1
		DiscreteDist customDist(vg);

		std::vector<double> probs_second = { 0.1, 0.3, 0.2, 0.1, 0.3 }; //With offset 1, these will be probabilities for 1, 2, 3, 4, 5 respectively
		int64_t min_second = 1;
		DynaPlex::VarGroup vg_second;
		vg_second.Add("type", "custom");
		vg_second.Add("probs", probs_second);
		vg_second.Add("offset", min_second); // Setting the offset so that the first probability is 1
		DiscreteDist customDist_second(vg_second);

		// Create a joint distribution
		// first / second		-1		0		1		2		Total  ---- indices
		//					1	0.01    0.05	0.03	0.01	0.1			0  5  10  15
		//					2	0.03	0.15	0.09	0.03	0.3			1  6  11  16
		//					3	0.02	0.1		0.06	0.02	0.2			2  7  12  17 
		//					4	0.01	0.05	0.03	0.01	0.1			3  8  13  18 
		//					5	0.03	0.15	0.09	0.03	0.3			4  9  14  19
		//				Total	0.1		0.5		0.3		0.1

		std::vector<DiscreteDist> dist = { customDist, customDist_second };
		JointDiscreteDist jointDist(dist);
		JointDiscreteDist jointDistCopy(customDist, customDist_second);

		double sum{ 0.0 };
		for (const auto& [qty, prob] : jointDist) {
			ASSERT_GE(qty, 0);  //so quantity should be non-negative.
			sum += prob;
		}
		ASSERT_NEAR(sum, 1.0, 0.001); // Total probability should be close to 1.

		double sum_copy{ 0.0 };
		for (const auto& [qty, prob] : jointDistCopy) {
			ASSERT_GE(qty, 0);  //so quantity should be non-negative.
			sum_copy += prob;
		}
		ASSERT_NEAR(sum_copy, 1.0, 0.001); // Total probability should be close to 1.

		std::vector<std::vector<int64_t>> JointQtys = jointDist.GetJointQtys();
		std::vector<int64_t> vec = { 0, 5 };
		std::vector<int64_t> vec1 = { 2, 3 };
		ASSERT_EQ(JointQtys[9], vec);
		ASSERT_EQ(JointQtys[17], vec1);

		ASSERT_NEAR(jointDist.ProbabilityAt(1), 0.03, 0.00001);
		ASSERT_NEAR(jointDist.ProbabilityAt(11), 0.09, 0.00001);
		ASSERT_NEAR(jointDist.ProbabilityAt(7), 0.1, 0.00001);
		ASSERT_NEAR(jointDist.ProbabilityAt(17), 0.02, 0.00001);
		ASSERT_EQ(jointDist.GetQtysForJointDist(3)[0], -1);
		ASSERT_EQ(jointDist.GetQtysForJointDist(5)[1], 1);
		ASSERT_EQ(jointDist.GetQtysForJointDist(12)[0], 1);
		ASSERT_EQ(jointDist.GetQtysForJointDist(16)[1], 2);
		ASSERT_EQ(jointDist.GetQtysForJointDist(19)[1], 5);

		ASSERT_NEAR(jointDistCopy.ProbabilityAt(1), 0.03, 0.00001);
		ASSERT_NEAR(jointDistCopy.ProbabilityAt(11), 0.09, 0.00001);
		ASSERT_NEAR(jointDistCopy.ProbabilityAt(7), 0.1, 0.00001);
		ASSERT_NEAR(jointDistCopy.ProbabilityAt(17), 0.02, 0.00001);
		ASSERT_EQ(jointDistCopy.GetQtysForJointDist(3)[0], -1);
		ASSERT_EQ(jointDistCopy.GetQtysForJointDist(5)[1], 1);
		ASSERT_EQ(jointDistCopy.GetQtysForJointDist(12)[0], 1);
		ASSERT_EQ(jointDistCopy.GetQtysForJointDist(16)[1], 2);
		ASSERT_EQ(jointDistCopy.GetQtysForJointDist(19)[1], 5);

		ASSERT_EQ(jointDist.FindPositionInJointQtys({ 2, 4 }), 18);
		ASSERT_EQ(jointDist.FindPositionInJointQtys({ -1, 5 }), 4);
		ASSERT_EQ(jointDistCopy.FindPositionInJointQtys({ -1, 1}), 0);
		ASSERT_EQ(jointDistCopy.FindPositionInJointQtys({ 0, 3 }), 7);
		ASSERT_EQ(jointDistCopy.FindPositionInJointQtys({ 1, 5 }), 14);

		ASSERT_NEAR(jointDist.ProbabilityAtFromQtys({ 2, 4 }), 0.01, 0.00001);
		ASSERT_NEAR(jointDist.ProbabilityAtFromQtys({ -1, 5 }), 0.03, 0.00001);
		ASSERT_NEAR(jointDistCopy.ProbabilityAtFromQtys({ -1, 1 }), 0.01, 0.00001);
		ASSERT_NEAR(jointDistCopy.ProbabilityAtFromQtys({ 0, 3 }), 0.1, 0.00001);
		ASSERT_NEAR(jointDistCopy.ProbabilityAtFromQtys({ 1, 5 }), 0.09, 0.00001);

		EXPECT_THROW(jointDist.GetQtysForJointDist(22), DynaPlex::Error);
		EXPECT_THROW(jointDistCopy.GetQtysForJointDist(20), DynaPlex::Error);
		EXPECT_THROW(jointDist.FindPositionInJointQtys({ 3, 6 }), DynaPlex::Error);
		EXPECT_THROW(jointDistCopy.FindPositionInJointQtys({ -2, 4 }), DynaPlex::Error);
	}

	TEST(jointdiscretedist, three_dist_joint_test) {
		// First distribution
		std::vector<double> probs1 = { 0.4, 0.6 }; // Probabilities for quantities 1 and 2
		int64_t min1 = 1;
		DynaPlex::VarGroup vg1;
		vg1.Add("type", "custom");
		vg1.Add("probs", probs1);
		vg1.Add("offset", min1);
		DiscreteDist dist1(vg1);

		// Second distribution
		std::vector<double> probs2 = { 0.5, 0.5 }; // Probabilities for quantities 3 and 4
		int64_t min2 = 3;
		DynaPlex::VarGroup vg2;
		vg2.Add("type", "custom");
		vg2.Add("probs", probs2);
		vg2.Add("offset", min2);
		DiscreteDist dist2(vg2);

		// Third distribution
		std::vector<double> probs3 = { 0.7, 0.3 }; // Probabilities for quantities 5 and 6
		int64_t min3 = 5;
		DynaPlex::VarGroup vg3;
		vg3.Add("type", "custom");
		vg3.Add("probs", probs3);
		vg3.Add("offset", min3);
		DiscreteDist dist3(vg3);

		// Create a joint distribution from three distributions
		std::vector<DiscreteDist> dists = { dist1, dist2, dist3 };
		JointDiscreteDist jointDist(dists);

		double sum{ 0.0 };
		for (const auto& [qty, prob] : jointDist) {
			ASSERT_GE(qty, 0);  //so quantity should be non-negative.
			sum += prob;
		}
		ASSERT_NEAR(sum, 1.0, 0.001); // Total probability should be close to 1.

		std::vector<std::vector<int64_t>> JointQtys = jointDist.GetJointQtys();
		std::vector<int64_t> vec = { 1, 3, 5 };
		std::vector<int64_t> vec1 = { 2, 4, 6 };
		ASSERT_EQ(JointQtys[0], vec);
		ASSERT_EQ(JointQtys[7], vec1);

		// Test specific probabilities
		ASSERT_NEAR(jointDist.ProbabilityAtFromQtys({ 1, 3, 5 }), 0.14, 0.00001);
		ASSERT_NEAR(jointDist.ProbabilityAtFromQtys({ 2, 4, 6 }), 0.09, 0.00001);

		// Test finding positions
		ASSERT_EQ(jointDist.FindPositionInJointQtys({ 1, 3, 6 }), 1);
		ASSERT_EQ(jointDist.FindPositionInJointQtys({ 2, 4, 5 }), 6);

		// Test getting quantities for a given position in joint distribution
		std::vector<int64_t> vec2 = { 1, 3, 6 };
		std::vector<int64_t> vec3 = { 2, 3, 6 };
		ASSERT_EQ(jointDist.GetQtysForJointDist(1), vec2);
		ASSERT_EQ(jointDist.GetQtysForJointDist(5), vec3);

		// Testing for expected exceptions
		EXPECT_THROW(jointDist.GetQtysForJointDist(10), DynaPlex::Error);
		EXPECT_THROW(jointDist.FindPositionInJointQtys({ 4, 1, 3 }), DynaPlex::Error);
	}

} // namespace DynaPlex::Tests
