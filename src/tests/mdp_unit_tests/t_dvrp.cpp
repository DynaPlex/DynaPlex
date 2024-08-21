#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {

    TEST(dynamic_vrp, mdp_config_0) {
        std::string model_name = "dynamic_vrp"; // this should match the id and namespace name discussed earlier
        std::string config_name = "mdp_config_0.json";
        Tester tester{};
        tester.ExecuteTest(model_name, config_name);
    }
}