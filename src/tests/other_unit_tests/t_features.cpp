#include "dynaplex/features.h"
#include "dynaplex/modelling/queue.h"
#include <gtest/gtest.h>

namespace DynaPlex::Tests {

    TEST(Features, AddIndividualItems) {
        std::vector<float> storage(5);
        DynaPlex::Features features(storage);

        features.Add(5);
        features.Add(5.5);
        features.Add(static_cast<float>(6));
        features.Add(true);
        features.Add(false);
        EXPECT_EQ(features[0], 5.0f);
        EXPECT_EQ(features[1], 5.5f);
        EXPECT_EQ(features[2], 6.0f);
        EXPECT_EQ(features[3], 1.0f);
        EXPECT_EQ(features[4], 0.0f);
        EXPECT_TRUE(features.IsFilled());
    }

    TEST(Features, AddQueue) {
        std::vector<float> storage(10);
        DynaPlex::Features features(storage);

        DynaPlex::Queue<int64_t> queue;
        
        queue.push_back(123);
        queue.push_back(12);

        features.Add(queue);


        EXPECT_EQ(storage[0], 123.0f);
        EXPECT_EQ(storage[1], 12.0f);

    }
    
    TEST(Features, AddContainer) {
        std::vector<float> storage(10);
        DynaPlex::Features features({ &storage[1],4 });

        std::vector<int64_t> intContainer = { 1, 2, 3 };
        
        features.Add(intContainer);

        EXPECT_EQ(features[0], 1.0f);
        EXPECT_EQ(features[1], 2.0f);
        EXPECT_EQ(features[2], 3.0f);
        EXPECT_EQ(storage[1], 1.0f);
        EXPECT_EQ(storage[2], 2.0f);
        EXPECT_EQ(storage[3], 3.0f);


    }
    
  

    TEST(Features, OverflowError) {
        std::vector<float> storage(2);
        DynaPlex::Features features(storage);

        features.Add(1.0f);
        features.Add(2.0f);
        features.Add(3.0f);
        EXPECT_FALSE(features.IsFilled());
    }

    TEST(Features, OutOfRangeError) {
        std::vector<float> storage(5);
        DynaPlex::Features features(storage);

        features.Add(5);
        features.Add(6);

        EXPECT_NO_THROW(features[0]);
        EXPECT_NO_THROW(features[1]);
        EXPECT_THROW(features[-2], Error);

        EXPECT_THROW(features[2], Error);
    }
} // namespace DynaPlex

