Unit testing
============


DynaPlex includes googletest, a well-known and well-documented test framework. Using tests to check whether code is following intended logic is a good way to verify that an MDP implements all required logic and to quickly identify oversights.

If compilation in the previous step was successful, it's time to set up a googletest for the new MDP:

1. Create a new file in ``src/tests/mdp_unit_tests`` named appropriately (e.g., ``t_mdp_id.cpp``), where ``mdp_id`` matches the directory name and the registration name of the newly added MDP.

2. Add some content that tests the new MDP. The following is a good start (note ``mdp_id`` should be changed twice to the id of the MDP you are testing for!). You can add other tests later:

.. code-block:: cpp

   // file: src/tests/mdp_unit_tests/t_mdp_id.cpp:
   #include <gtest/gtest.h>
   #include "testutils.h" // for ExecuteTest

   namespace DynaPlex::Tests {    
       TEST(mdp_id, mdp_config_0) {
           std::string model_name = "mdp_id"; // this should match the id and namespace name discussed earlier
           std::string config_name = "mdp_config_0.json";
           Tester tester{};
           tester.ExecuteTest(model_name, config_name);
       }
       // other tests can be added later. 
   }

3. If wanted, modify the configuration file for testing the new MDP. Ensure the old ``mdp_config_0.json`` name is still valid but update the id:

.. code-block:: json

   {
     "id": "mdp_id",
     // other variables should be left unchanged, as we didn't make any changes to the old MDP except the id under which it is registered. 
   }

Compiling and running ``mdp_unit_tests.exe`` should give some results. If starting from a working MDP (like ``lost_sales``), all tests should pass since all we did was register existing logic under a new name. If starting from ``empty_example``, tests will fail, since many functions in that MDP throw exceptions, but this can be solved by inputting appropriate logic everywhere.

This default test works as follows. During compilation, the JSON files in ``models/models`` get copied to a subdirectory in ``DYNAPLEX_IO_ROOT_DIR/IO_DynaPlex/defaults``.

Now, when the test executes, ``ExecuteTest`` does the following:
- Loads JSON from ``IORootDir/IO_DynaPlex/defaults/model_name/mdp_config_name``
- Attempts to configure MDP from that JSON file
- Sets a random policy
- Performs a range of tests using the MDP and the Policy, i.e., checks whether a number of simulation steps can be run and whether all things work appropriately.

**Note:** A .json file is one way to configure an MDP, but MDPs may also be directly configured from C++ code (via a VarGroup, see ``../src/tests/other_unit_tests/t_model_provider.cpp``) or via a dict object in Python.