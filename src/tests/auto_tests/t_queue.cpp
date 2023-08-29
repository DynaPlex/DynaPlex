#include <iostream>
#include <deque>
#include <gtest/gtest.h>
#include <vector>
#include <iterator>

#include "dynaplex/modelling/queue.h"

namespace DynaPlex::Tests {

	using namespace DynaPlex::Modelling;
	TEST(queue, basics) {

		// Construct an empty Queue
		Queue<int64_t> queue{};

		// Test push_back and front
		queue.push_back(1);
		EXPECT_EQ(queue.front(), 1);

		// Test push_back and back
		queue.push_back(2);
		EXPECT_EQ(queue.back(), 2);

		// Test size and capacity
		EXPECT_EQ(queue.Capacity(), 4); // As defined in the default constructor
		EXPECT_EQ(queue.front(), 1);

		// Test pop_front
		int64_t poppedValue = queue.pop_front();
		EXPECT_EQ(poppedValue, 1);
		EXPECT_EQ(queue.front(), 2);

		// Check for exception when popping from an empty queue
		queue.pop_front();
		EXPECT_THROW(queue.pop_front(), DynaPlex::Error);

		// Test copy constructor
		Queue<int64_t> q1 = { 1, 2, 3 };
		Queue<int64_t> q2(q1);
		EXPECT_EQ(q1, q2); // Using the == operator we defined

		// Test initializer list constructor
		Queue<int64_t> q3 = { 4, 5, 6 };
		q3.push_back(7);
		EXPECT_EQ(q3.back(), 7);

		// Test move constructor
		Queue<int64_t> q4(std::move(q3));
		EXPECT_EQ(q4.back(), 7);
		EXPECT_TRUE(q3.IsEmpty());

		// Test iterators
		int64_t sum = 0;
		for (int64_t val : q4) {
			sum += val;
		}
		EXPECT_EQ(sum, 4 + 5 + 6 + 7);
	}


	TEST(queue, basics2) {
		Queue<int64_t> q1(3, 1);
		// Push items into the queue
		q1.push_back(2);
		q1.push_back(3);  // Queue should now be: { 1, 1, 1, 2, 3 }

		// Validate the content using iterators
		std::vector<int64_t> expected1 = { 1, 1, 1, 2, 3 };
		EXPECT_TRUE(std::equal(q1.begin(), q1.end(), expected1.begin()));

		// Pop items
		q1.pop_front();  // Remove first 1
		q1.pop_front();  // Remove second 1

		// Validate again
		std::vector<int64_t> expected2 = { 1, 2, 3 };
		EXPECT_TRUE(std::equal(q1.begin(), q1.end(), expected2.begin()));

		// Check front and back methods
		EXPECT_EQ(q1.front(), 1);
		EXPECT_EQ(q1.back(), 3);

		// Create another queue using an initializer list
		Queue<int64_t> q2 = { 0, 0, 0, 1, 2, 3 };
		EXPECT_TRUE(std::equal(q2.begin(), q2.end(), std::vector<int64_t>{0, 0, 0, 1, 2, 3}.begin()));

		// Push and pop operations for q2
		q2.push_back(4);
		q2.push_back(5);  // { 0, 0, 0, 1, 2, 3, 4, 5 }

		q2.pop_front();  // Remove first 0
		q2.pop_front();  // Remove second 0
		q2.pop_front();  // Remove third 0

		// Validate again
		std::vector<int64_t> expected3 = { 1, 2, 3, 4, 5 };
		EXPECT_TRUE(std::equal(q2.begin(), q2.end(), expected3.begin()));

		// Test the copy constructor
		Queue<int64_t> q3 = q2;
		EXPECT_TRUE(q3 == q2);

		// Test pushing to a queue after copying
		q3.push_back(6);
		q3.push_back(7);

		// The two queues should now be different
		EXPECT_TRUE(q3 != q2);
	}

} // namespace DynaPlex::Tests
