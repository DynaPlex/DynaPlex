Tester class reference
======================

.. cpp:class:: Tester

   The Tester class is designed to validate and test the implementation of various aspects of the MDP and its policies. It primarily focuses on ensuring the correctness of the MDP's configuration, state management, policy implementation, and the overall interaction between these components.

   .. cpp:function:: void Tester::ExecuteTest(const std::string& model_name, const std::string& mdp_config_name, const std::string& policy_config_name)

      Executes a test on a specific MDP model with given configuration and policy settings. This function loads the MDP and policy configurations, initiates the state, and runs a series of checks and simulations to ensure the correct implementation of the MDP and its policies.

      :param const std::string& model_name: Name of the MDP model to be tested.
      :param const std::string& mdp_config_name: Configuration name for the MDP.
      :param const std::string& policy_config_name: Configuration name for the policy. If empty, a default policy is used.

      **Usage Example**

      .. code-block:: cpp

         // Example of using ExecuteTest
         Tester tester;
         tester.ExecuteTest("model_name", "mdp_config", "policy_config");
