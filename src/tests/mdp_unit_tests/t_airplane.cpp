#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {

    TEST(airplane, mdp_config_0) {
        std::string model_name = "airplane"; // this should match the id and namespace name discussed earlier
        std::string config_name = "mdp_config_0.json";
        std::string policy_config_name = "policy_config_0.json";
        Tester tester{};
        tester.ExecuteTest(model_name, config_name, policy_config_name);
    }
}